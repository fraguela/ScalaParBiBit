/*
 * FileWorker.h
 *
 *  Created on: 16/03/2017
 *      Author: jorge
 */

#ifndef FILEWORKER_H_
#define FILEWORKER_H_

#include "Bicluster.h"
#include "Options.h"

class FileWorker {
public:
	FileWorker();
	FileWorker(Options *options, int numGenes, int numSamples, int numLevels);
	virtual ~FileWorker();

	inline void printHeader(int level){
		ofstream f(_outputFiles[level-1].c_str(), ofstream::out);
		f << "NumOfRows;NumOfColumns;Rows;Columns\n";
	}

	// Return the number of printed biclusters
	inline int printBiclusters(vector<Bicluster *> *biclusterVec, int level){
		if(_readNames){
			return _printReadNames(biclusterVec, level);
		} else {
			return _printIniNames(biclusterVec, level);
		}
	}

private:
	int _printReadNames(vector<Bicluster *> *biclusterVec, int level);
	int _printIniNames(vector<Bicluster *> *biclusterVec, int level);
	void _readGenes(string fileName, int num);
	void _readSamples(string fileName, int num);

	// Indicate whether the name of the genes and samples was defined
	bool _readNames;

	// Lists with the information (if not input files, the names will be just GeneX, SampleX)
	string *_genesNames;
	string *_samplesNames;

	// Output files for each level
	string *_outputFiles;
};

#endif /* FILEWORKER_H_ */
