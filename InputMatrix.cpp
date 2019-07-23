/*
 * InputMatrix.cpp
 *
 *  Created on: 10/03/2017
 *      Author: jorge
 */

#include "InputMatrix.h"

InputMatrix::InputMatrix() {
	_numGenes = -1;
	_numSamples = -1;
	_numEncodedCols = -1;

	_vals = NULL;
	_encodedVals = NULL;
}

InputMatrix::InputMatrix(ArffData *data) {
	_numSamples = data->num_instances();
	_numGenes = data->num_attributes();
	_numEncodedCols = _numSamples/32+1;

	Utils::log("Loading data with %d samples and %d genes\n", _numSamples, _numGenes);

	_vals = new int[_numGenes*_numSamples];
	_encodedVals = new uint32_t[_numGenes*_numEncodedCols];

	// Copy the elements
	for(int i=0; i<_numSamples; i++){
		ArffInstance *inst = data->get_instance(i);
		for(int j=0; j<_numGenes; j++){
			ArffValue *val = inst->get(j);
			_vals[i+j*_numSamples] = (int32)*val;
		}
	}
}

// This is used by processes that do not read the input
InputMatrix::InputMatrix(int numGenes, int numSamples){
	_numSamples = numSamples;
	_numGenes = numGenes;
	_numEncodedCols = _numSamples/32+1;

	_vals = NULL; // Not necessary in this case
	_encodedVals = new uint32_t[_numGenes*_numEncodedCols];
}

InputMatrix::~InputMatrix() {
	if(_vals != NULL){
		delete [] _vals;
	}

	if(_encodedVals != NULL){
		delete [] _encodedVals;
	}
}

void InputMatrix::discretizeMatrix(int level){
	uint32_t encodedVal;
	int *iterVals = _vals;
	uint32_t *auxEncoded = _encodedVals;
	for(int i=0; i<_numGenes; i++){
		for(int j=0; j<_numEncodedCols-1; j++){
			encodedVal = 0;
			for(int k=0; k<32; k++){
				encodedVal *= 2;
				if(*iterVals >= level)
					encodedVal += 1;
				iterVals++;
			}
			*auxEncoded = encodedVal;
			auxEncoded++;
		}
		encodedVal = 0;
		for(int k=(_numEncodedCols-1)*32; k<_numSamples; k++){
			encodedVal *= 2;
			if(*iterVals >= level)
				encodedVal += 1;
			iterVals++;
		}
		for(int k=_numSamples; k<_numEncodedCols*32; k++){
			encodedVal *= 2;
		}
		*auxEncoded = encodedVal;
		auxEncoded++;
	}
}

int InputMatrix::bcastEncodedValues(){
	return MPI_Bcast(_encodedVals, _numGenes*_numEncodedCols*sizeof(uint32_t), MPI_BYTE, 0, MPI_COMM_WORLD);
}
