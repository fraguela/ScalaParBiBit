/*
 * FileWorker.cpp
 *
 *  Created on: 16/03/2017
 *      Author: jorge
 */

#include "FileWorker.h"

FileWorker::FileWorker() {
	_genesNames = NULL;
	_samplesNames = NULL;
	_outputFiles = NULL;
	_readNames = false;
}

FileWorker::FileWorker(Options *options, int numGenes, int numSamples, int numLevels) {

	_outputFiles = new string[numLevels];

	_readNames = true;

	if(options->existRowFile()){
		_genesNames = new string[numGenes];
		_readGenes(options->getRowFileName(), numGenes);
	} else {
		_genesNames = NULL;
		_readNames = false;
	}

	if(options->existColFile()){
		_samplesNames = new string[numSamples];
		_readSamples(options->getColFileName(), numSamples);
	} else {
		_samplesNames = NULL;
		_readNames = false;
	}

	string fileName;
	for(int i=0; i<numLevels; i++){
		fileName = options->getOutputFileName();
		fileName.append("_");
		fileName.append(to_string((long long)(i+1)));
		fileName.append(".txt");

		_outputFiles[i] = fileName;
	}
}

FileWorker::~FileWorker() {
	if(_genesNames != NULL){
		delete [] _genesNames;
	}

	if(_samplesNames != NULL){
		delete [] _samplesNames;
	}

	if(_outputFiles != NULL){
		delete [] _outputFiles;
	}
}

int FileWorker::_printReadNames(vector<Bicluster *> *biclusterVec, int level){

	ofstream f(_outputFiles[level-1].c_str(), ofstream::out | ofstream::app);

	Bicluster *b;
	int numGenes, numPattern, samplesPrinted, iterPattern;
	int *genes;
	uint32_t *pattern;
	int numPrinted = 0;

	for(uint32_t i=0; i<biclusterVec->size(); i++){
		b = biclusterVec->at(i);

		numGenes = b->getNumGenes();
		numPattern = b->getPatternOnes();

		f << numGenes << ";" << numPattern << ";";

		for(int j=0; j<numGenes-1; j++){
			f << _genesNames[b->getGene(j)] << ",";
		}
		f << _genesNames[b->getGene(numGenes-1)] << ";";

		pattern = b->getPattern();
		iterPattern = 0;
		samplesPrinted = 0;
		while(samplesPrinted < numPattern){
			uint32_t aux = 0x80000000;
			for(int j=0; j<32; j++){
				if(pattern[iterPattern] & aux){
					if(samplesPrinted < numPattern-1){
						f << _samplesNames[iterPattern*32+j] << ",";
					} else {
						f << _samplesNames[iterPattern*32+j] << "\n";
					}

					samplesPrinted++;
				}

				aux /= 2;
			}
			iterPattern++;
		}

		numPrinted++;
	}

	f.close();
	return numPrinted;
}

int FileWorker::_printIniNames(vector<Bicluster *> *biclusterVec, int level){

	ofstream f(_outputFiles[level-1].c_str(), ofstream::out | ofstream::app);

	Bicluster *b;
	int numGenes, numPattern, samplesPrinted, iterPattern;
	int *genes;
	uint32_t *pattern;
	int numPrinted = 0;

	for(uint32_t i=0; i<biclusterVec->size(); i++){
		b = biclusterVec->at(i);

		numGenes = b->getNumGenes();
		numPattern = b->getPatternOnes();

		f << numGenes << ";" << numPattern << ";";

		for(int j=0; j<numGenes-1; j++){
			f << "G" << b->getGene(j) << ",";
		}
		f << "G" << b->getGene(numGenes-1) << ";";

		pattern = b->getPattern();
		iterPattern = 0;
		samplesPrinted = 0;
		while(samplesPrinted < numPattern){
			uint32_t aux = 0x80000000;
			for(int j=0; j<32; j++){
				if(pattern[iterPattern] & aux){
					if(samplesPrinted < numPattern-1){
						f << "S" << iterPattern*32+j << ",";
					} else {
						f << "S" << iterPattern*32+j << "\n";
					}

					samplesPrinted++;
				}

				aux /= 2;
			}
			iterPattern++;
		}

		numPrinted++;
	}

	f.close();
	return numPrinted;
}

void FileWorker::_readGenes(string fileName, int num){
	ifstream f(fileName.c_str());

	string aux;

	for(int i=0; i<num; i++){
		if(!getline(f, _genesNames[i])){
			Utils::exit("Not enough elements (less than %d) in file %s\n", i, fileName.c_str());
		}
	}

	f.close();
}

void FileWorker::_readSamples(string fileName, int num){
	ifstream f(fileName.c_str());

	for(int i=0; i<num; i++){
		if(!getline(f, _samplesNames[i])){
			Utils::exit("Not enough elements (less than %d) in file %s\n", i, fileName.c_str());
		}
	}

	f.close();
}

