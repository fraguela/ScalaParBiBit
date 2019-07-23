/*
 * Options.h
 *
 *  Created on: 10/03/2017
 *      Author: jorge
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "Utils.h"

class Options {
public:
	Options();
	virtual ~Options();

	/*virtual functions*/
	void printUsage();
	bool parse(int argc, char* argv[]);

	inline string getInputFileName(){
		return _inputFileName;
	}

	inline string getOutputFileName(){
		return _outFileName;
	}

	inline string getRowFileName(){
		return _rowFileName;
	}

	inline string getColFileName(){
		return _colFileName;
	}

	inline bool existRowFile(){
		return (_rowFileName.size() > 0);
	}

	inline bool existColFile(){
		return (_colFileName.size() > 0);
	}

	inline int getMaxVal(){
		return _maxVal;
	}

	inline int getMinRows(){
		return _minRows;
	}

	inline int getMinColumns(){
		return _minColumns;
	}

        inline int getChunkSize() const noexcept { return _chunkSize; }

	inline int getNumTh(){
		return _numTh;
	}

private:
	string _inputFileName;
	string _outFileName;
	string _rowFileName;
	string _colFileName;

	int _maxVal; // maximum value in the discretized dataset. From this value, ParBibit will binarize the dataset generating max_value different ones.
	int _minRows; // minimum number of rows allowed in a valid bicluster
	int _minColumns; // minimum number of columns allowed in a valid bicluster
        int _chunkSize;  // size of the chunks to process
	int _numTh;
};

#endif /* OPTIONS_H_ */
