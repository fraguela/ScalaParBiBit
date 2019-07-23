#include <atomic>
#include "Bicluster.h"
#include "FileWorker.h"
#include "arff_parser.h"
#include "ServerState.h"
#include "MurmurHash.h"
#include "UnorderedVarSet.h"
#include "ThreadHandler.h"

namespace {
  
  vector<Bicluster *> biclusterVec;
  int MinRows;
  mutex MutexVec;
  
  int BiclustersPerChunk = 0;
  int InitBiclusterSize = 0; ///< Size of an initial Bicluster (in uint32_t)
  constexpr bool DynamicInitBalance = true;
  std::atomic<int> AtomicCurrentGen{0};
  GlobalServerState_t GlobalServerState;

  struct VecAccessor {
    uint32_t * auxPattern_;
    static int PatternLength;
    
    VecAccessor(uint32_t * auxPattern) :
    auxPattern_(auxPattern)
    {}
    
    /// Returns space required to store the object
    static size_t size() noexcept { return sizeof(uint32_t) * PatternLength; }
    
    /// Stores the object in p.
    void fill(void *p) const noexcept {
      memcpy(p, auxPattern_, size());
    }
    
    /// Return hash associated to the object
    size_t hash() const noexcept {
      return MurmurHash64(auxPattern_, size());
    }
    
    /// Return whether the stored value equals the representation stored from the pointer
    bool matches(void *p) const noexcept {
      return !memcmp(p, auxPattern_, size());
    }
    
  };
  
  int VecAccessor::PatternLength;
}

// Routines to be performed by different threads

// In this case the work per bicluster can be different (some genes will be discarded easier than other) so we apply a dynamic distribution
void completeBiclustersTh(InputMatrix * const mat, uint32_t * const buffer, std::atomic<int> * const numIters, const int numBiclusters, const int biclustersPerBlock){
  
  int numGenes = mat->getNumGenes();
  int numSamples = mat->getNumSamples();
  int patternLength = numSamples/32+1;
  int myNumIters, gen1, gen2;
  int bufSize = 2 * MinRows;
  int *buf = static_cast<int *>(malloc(sizeof(int) * bufSize));
  vector<Bicluster *> localBiclusterVec;
  localBiclusterVec.reserve(std::min(biclustersPerBlock, 8));
  
  while(1){
    // Get the next bicluster to be analyzed from the shared variable
    myNumIters = numIters->fetch_add(biclustersPerBlock, std::memory_order_relaxed);
    
    // Check if there are more genes in the biclusters
    for(int i=0; (i<biclustersPerBlock) && (i+myNumIters < numBiclusters); i++){
      
      uint32_t * const src = buffer + (i+myNumIters) * InitBiclusterSize;
      Bicluster bicluster(src + 3, numSamples, src[0], src[1], src[2], buf, bufSize);
      
      for(int gen3=0; gen3<numGenes; gen3++){
        bicluster.insertGene(mat, gen3);
      }
      
      if(bicluster.getNumGenes() >= MinRows){
        localBiclusterVec.push_back(new Bicluster(bicluster));
      }
      
      bicluster.reset(buf, bufSize);
    }
    
    if(*numIters >= numBiclusters){
      break;
    }
  }
  
  free(buf);
  
  const size_t localSize = localBiclusterVec.size();
  if (localSize) {
    MutexVec.lock();
    biclusterVec.reserve(biclusterVec.size() + localSize);
    biclusterVec.insert(biclusterVec.end(), localBiclusterVec.begin(), localBiclusterVec.end());
    MutexVec.unlock();
  }
}

uint32_t *assignCompletion(uint32_t *cur_vector, int numBiclusters, InputMatrix * const mat)
{
  if (!GlobalServerState.findServer(cur_vector, numBiclusters)) {
    std::atomic<int> tmp {0};
    completeBiclustersTh(mat, cur_vector, &tmp, numBiclusters, numBiclusters);
  }
  return new uint32_t[BiclustersPerChunk * InitBiclusterSize];
}

// In this case the computation for each bicluster is the same so we apply a static distribution
// The arrays must have enough memory to store all the necesary data at the beginning
// If memory is exceeded then the error is set to 1
void iniBiclustersTh(const int tid, const int numThreads, uint32_t minCols,
                     InputMatrix * const mat, UnorderedVarSet<VecAccessor> * const patternsSet){
  
	int numGenes = mat->getNumGenes();
	int patternLength = mat->getNumSamples()/32+1;
        int increment = 2 * tid + 1;
        Bicluster bicluster(mat);
        uint32_t * const auxPattern = bicluster.getPattern();

        uint32_t *cur_vector = new uint32_t[BiclustersPerChunk * InitBiclusterSize];
        int insertion_pos = 0;
        patternsSet->thread_init();
        VecAccessor vec_acc(auxPattern);

	// Search for biclusters
        for(int gen1=tid;
            gen1<numGenes;
            gen1 = AtomicCurrentGen.fetch_add(1, std::memory_order_relaxed)){
		increment = 2 * numThreads - increment;
		for(int gen2=gen1+1; gen2<numGenes; gen2++){
			bicluster.redefine(mat, gen1, gen2);
			if(bicluster.getPatternOnes() >= minCols){
			    if( patternsSet->insert(vec_acc) ){
                              if (insertion_pos == BiclustersPerChunk) {
                                cur_vector = assignCompletion(cur_vector, insertion_pos, mat);
                                insertion_pos = 0;
                              }
                              uint32_t * __restrict__ dest = cur_vector + insertion_pos * InitBiclusterSize;
                              dest[0] = static_cast<uint32_t>(gen1);
                              dest[1] = static_cast<uint32_t>(gen2);
                              dest[2] = static_cast<uint32_t>(bicluster.getPatternOnes());
                              memcpy(dest + 3, auxPattern, patternLength * sizeof(uint32_t));
                              insertion_pos++;
			    }
			}
		}
	}
  
        if (insertion_pos) {
                cur_vector = assignCompletion(cur_vector, insertion_pos, mat);
        }
        delete [] cur_vector;
}

/**
 * The main subroutine.  Parses the input parameters and executes the program
 * accordingly.
 */
int main(int argc, char *argv[]) {

  int provided, rank, numP;
  
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
  if(provided < MPI_THREAD_SERIALIZED) {
    Utils::exit("MPI_Init_thread MPI_THREAD_SERIALIZED not available\n");
  }

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numP);

	double initime, etime, total_time;

#ifdef BENCHMARKING
	double stime, read_time, disc_time = 0.0, creation_time = 0.0, adding_time=0.0, print_time = 0.0, comm_time = 0.0;
#endif

	Options* const options = new Options();

	MPI_Barrier(MPI_COMM_WORLD);
	initime = Utils::getSysTime();

	// Parse the arguments
	if (!options->parse(argc, argv)) {
		options->printUsage();
		return 0;
	}

	int numTh = options->getNumTh();
        BiclustersPerChunk = options->getChunkSize();
  
	InputMatrix *mat;
	int matDims[2];

	if(!rank){
          const char * const version_name = "dyn_distr_thread_pool";

          Utils::log("Executing with %d processes and %d threads. Version %s Chunksize=%d\n", numP, numTh, version_name, BiclustersPerChunk);

          ArffParser *parser = new ArffParser(options->getInputFileName().c_str());
          ArffData *data = parser->parse();
          
          // Read from input files
          mat = new InputMatrix(data);
          delete parser;

          matDims[0] = mat->getNumGenes();
          matDims[1] = mat->getNumSamples();

          Utils::log("Number of rows: %d\n", matDims[0]);
          Utils::log("Number of columns: %d\n", matDims[1]);
	}

#ifdef BENCHMARKING
	etime = Utils::getSysTime();
	read_time = etime-initime;
	Utils::log("Process %d: Read and transposed the inputs in %.2f seconds\n", rank, read_time);
	stime = Utils::getSysTime();
#endif

	if(MPI_Bcast(matDims, 2, MPI_INT, 0, MPI_COMM_WORLD)){
		Utils::exit("Error in process %d broadcasting the matrix dimensions\n", rank);
	}

	if(rank > 0){
		mat = new InputMatrix(matDims[0], matDims[1]);
	}

	FileWorker *fworker = new FileWorker(options, matDims[0], matDims[1], options->getMaxVal());

	const int patternLength = matDims[1]/32+1;
        VecAccessor::PatternLength = patternLength;
  
	if(MPI_Bcast(mat->getAllEncodedVals(), matDims[0]*patternLength, MPI_UINT32_T, 0, MPI_COMM_WORLD)){
		Utils::exit("Error in process %d broadcasting the matrix dimensions\n", rank);
	}

	MinRows = options->getMinRows();
	uint32_t minCols = options->getMinColumns();

#ifdef BENCHMARKING
	etime = Utils::getSysTime();
	read_time += etime-stime;
	Utils::log("Process %d: Broadcasted the input in %.2f seconds (included in read time)\n", rank, etime-stime);
	stime = Utils::getSysTime();
#endif

        InitBiclusterSize = patternLength + 3;
        if (!rank) {
          GlobalServerState.initialize(numP - 1, InitBiclusterSize, true);
        }

	// Compute each level
	int i=options->getMaxVal();
	for(; i>0; i--){
          
#ifdef BENCHMARKING
		stime = Utils::getSysTime();
#endif

		if(!rank){
			Utils::log("Level %d\n", i);
			// Encode
			mat->discretizeMatrix(i);
#ifdef BENCHMARKING
			etime = Utils::getSysTime();
			disc_time += etime-stime;
			Utils::log("Process 0: Discretized the matrix at level %d in %.2f seconds\n", i, etime-stime);
			stime = Utils::getSysTime();
#endif
		}

		// Replicate the discretized matrix
		if(mat->bcastEncodedValues()){
			Utils::exit("Error broadcasting the discretized matrix\n");
		}

                int setSize = 0;
          
#ifdef BENCHMARKING
		etime = Utils::getSysTime();
		comm_time += etime-stime;
		Utils::log("Process %d: Broadcasted the discretized matrix at level %d in %.2f seconds\n", rank, i, etime-stime);
		stime = etime;
#endif

		if(!rank) {
                  vector<thread> threads;
                  UnorderedVarSet<VecAccessor> patternsSet( (matDims[0] / 20) * matDims[0] ); //Reserve buckets for 5% of coincidences
                  
                  AtomicCurrentGen.store(numTh);
                  for(int th=0; th<numTh; th++){
                    threads.push_back(thread(iniBiclustersTh, th, numTh, minCols, mat, &patternsSet));
                  }

                  for(int th=0; th<numTh; th++){
                    threads[th].join();
                  }

                  setSize = patternsSet.size();
                  
                  GlobalServerState.finish();
                } else {
                  MPI_Status status;
                  int count;
                  ThreadHandler thread_handler(numTh-1);

                  while(1) {
                    std::atomic<int> numIters {0};
                    uint32_t * const buffer = new uint32_t[BiclustersPerChunk * InitBiclusterSize];
                    GlobalServerState.push_back_local_buffer(buffer);
                    MPI_Recv(buffer, BiclustersPerChunk * InitBiclusterSize, MPI_UINT32_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    if (status.MPI_TAG == GlobalServerState_t::FinishTag) {
                      break;
                    }
                    MPI_Get_count(&status, MPI_UINT32_T, &count);
                    const int myNumBi = count / InitBiclusterSize;
                    int biclustersPerBlock = myNumBi/(10*numTh);
                    if(!biclustersPerBlock){
                      biclustersPerBlock = 1;
                    }
                    
                    // Each process launches several threads to analyze the biclusters
                    thread_handler.setFunction(completeBiclustersTh, mat, buffer, &numIters, myNumBi, biclustersPerBlock);
                    thread_handler.launchTheads();
                    completeBiclustersTh(mat, buffer, &numIters, myNumBi, biclustersPerBlock);
                    thread_handler.wait();

                    MPI_Send(&count, 0, MPI_INT, 0, GlobalServerState_t::FinishTag, MPI_COMM_WORLD);
                  }
                  
                }

#ifdef BENCHMARKING
		MPI_Barrier(MPI_COMM_WORLD);
		etime = Utils::getSysTime();
		creation_time += etime-stime;
		Utils::log("Process %d: Initialized %d biclusters at level %d in %.2f seconds\n", rank, setSize, i, etime-stime);
#endif
		setSize = biclusterVec.size();
#ifdef BENCHMARKING
		Utils::log("Process %d: Completed %d biclusters at level %d in %.2f seconds\n", rank, setSize, i, etime-stime);
		stime = etime;
#endif

		// Each process prints its biclusters
		if(!rank){
			fworker->printHeader(i);
			setSize = fworker->printBiclusters(&biclusterVec, i);
		}

		for(int p=1; p<numP; p++){
			MPI_Barrier(MPI_COMM_WORLD);
			if(rank == p){
				setSize = fworker->printBiclusters(&biclusterVec, i);
			}
		}

#ifdef BENCHMARKING
		etime = Utils::getSysTime();
		print_time += etime-stime;
		Utils::log("Process %d: Printed %d biclusters at level %d in %.2f seconds\n", rank, setSize, i, etime-stime);
		stime = etime;
#endif

		biclusterVec.clear();

                GlobalServerState.clear();
	}

	delete fworker;
	delete mat;

	etime = Utils::getSysTime();
	total_time = etime-initime;

	if(rank == numP-1){ // Process 0 is special and partial times would not be representative
#ifdef BENCHMARKING
		Utils::log("Overall time: %.2f seconds (%.2f for input reading; %.2f for matrix discretization; %.2f for bicluster initialization; %.2f for bicluster completion; %.2f for printing; %.2f for communication)\n",
				total_time, read_time, disc_time, creation_time, adding_time, print_time, comm_time);
#else
		Utils::log("Overall time: %.2f seconds\n", total_time);
#endif
	}

	MPI_Finalize();
	return 0;
}
