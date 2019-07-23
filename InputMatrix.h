/*
 * InputMatrix.h
 *
 *  Created on: 10/03/2017
 *      Author: jorge
 */

#ifndef INPUTMATRIX_H_
#define INPUTMATRIX_H_

#include "Utils.h"
#include "arff_data.h"

class InputMatrix {
public:
	InputMatrix();
	InputMatrix(ArffData *data);
	// This is used by processes that do not read the input
	InputMatrix(int numGenes, int numSamples);

	virtual ~InputMatrix();

	inline int getNumGenes(){
		return _numGenes;
	}

	inline int getNumSamples(){
		return _numSamples;
	}

	inline int getExtraCols(){
		return _numEncodedCols;
	}

	inline int getVal(int gene, int sample){
		return _vals[gene*_numSamples+sample];
	}

	inline int *getAllVals(){
		return _vals;
	}

	inline uint32_t getEncodedVal(int gene, int pos){
		return _encodedVals[gene*_numEncodedCols+pos];
	}

	inline uint32_t *getEncodedGene(int gene){
		return &_encodedVals[gene*_numEncodedCols];
	}

	inline uint32_t *getAllEncodedVals(){
		return _encodedVals;
	}

	void discretizeMatrix(int level);

	int bcastEncodedValues();

private:
	int _numGenes;
	int _numSamples;
	int _numEncodedCols;

	// Rows for samples and columns for genes (opposite to arff)
	int *_vals;
	uint32_t *_encodedVals;
};

#endif /* INPUTMATRIX_H_ */
