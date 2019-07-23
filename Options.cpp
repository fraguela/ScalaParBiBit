/*
 * Options.cpp
 *
 *  Created on: 10/03/2017
 *      Author: jorge
 */

#include "Options.h"

Options::Options() {
  _maxVal = -1;
  _minRows = -1;
  _minColumns = -1;
  _numTh = 1;
  _chunkSize = 1024;
}

Options::~Options() {

}

void Options::printUsage()
{
	fprintf(stderr, "\nUsage: %s -i inputFile -o outputFile -r rowFile -c columnFile -mv maxValue -mr minRows -mc minColumns -d -p -C chunksize\n", PROGRAM_NAME);
	/*the file input options*/
	fprintf(stderr,
			"\t-i <string> inputFile (.arff file)\n");
	fprintf(stderr,
			"\t-o <string> outputFile\n");
	fprintf(stderr,
			"\t-r <string> rowFile (one-column file with the names of all the rows, following the order they appear in the dataset of inputFile)\n");
	fprintf(stderr,
			"\t-c <string> colFile (one-column file with the names of all the columns, following the order they appear in the dataset of inputFile)\n");
	fprintf(stderr, "\t-mv <int> (the maximum value in the discretized dataset. From this value, ParBibit will binarize the dataset generating max_value different ones.)\n");
	fprintf(stderr, "\t-mr <int> (minimum number of rows allowed in a valid bicluster)\n");
	fprintf(stderr, "\t-mc <int> (minimum number of columns allowed in a valid bicluster)\n");
	fprintf(stderr, "\t-t <int> (number of threads (by default: %d)\n", _numTh);
        fprintf(stderr, "\t-C <int> (processing chunk size (by default: %d)\n", _chunkSize);
	fprintf(stderr, "\t-h <print out the usage of the program)\n\n");
}

bool Options::parse(int argc, char* argv[])
{
	int intVal;
	int argind = 1;

	if (argc < 11) {
		Utils::log("Not enough parameters\n");
		return false;
	}

	/*check the help*/
	if (!strcmp(argv[argind], "-h") || !strcmp(argv[argind], "-?")) {
		return false;
	}

	/*print out the command line*/
	fprintf(stderr, "Command: ");
	for (int i = 0; i < argc; ++i) {
		fprintf(stderr, "%s ", argv[i]);
	}
	fputc('\n', stderr);

	while (argind < argc) {
		/*single-end sequence files*/
		if (!strcmp(argv[argind], "-i")) {
			argind++;
			if (argind < argc) {
				_inputFileName = argv[argind];
				argind++;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-o")) {
			argind++;
			if (argind < argc) {
				_outFileName = argv[argind];
				argind++;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-r")) {
			argind++;
			if (argind < argc) {
				_rowFileName = argv[argind];
				argind++;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-c")) {
			argind++;
			if (argind < argc) {
				_colFileName = argv[argind];
				argind++;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-mv")) {
			intVal = 1;
			argind++;
			if (argind < argc) {
				sscanf(argv[argind], "%d", &intVal);
				if (intVal <= 0){
					Utils::log("The maximum value in the discretized dataset must be higher than 0\n");
					return false;
				}

				argind++;
				_maxVal = intVal;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-mr")) {
			intVal = 1;
			argind++;
			if (argind < argc) {
				sscanf(argv[argind], "%d", &intVal);
				if (intVal < 2){
					Utils::log("The minimum number of rows per bicluster must be higher than 1\n");
					return false;
				}

				argind++;
				_minRows = intVal;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-mc")) {
			intVal = 1;
			argind++;
			if (argind < argc) {
				sscanf(argv[argind], "%d", &intVal);
				if (intVal < 2){
					Utils::log("The minimum number of columns per bicluster must be higher than 1\n");
					return false;
				}

				argind++;
				_minColumns = intVal;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
		} else if (!strcmp(argv[argind], "-t")) {
			intVal = 1;
			argind++;
			if (argind < argc) {
				sscanf(argv[argind], "%d", &intVal);
				if (intVal < 1){
					Utils::log("The number of threads must be higher than 0\n");
					return false;
				}

				argind++;
				_numTh = intVal;
			} else {
				Utils::log("not specify value for the parameter %s\n",
						argv[argind - 1]);
				return false;
			}
                } else if (!strcmp(argv[argind], "-C")) {
                  intVal = 1;
                  argind++;
                  if (argind < argc) {
                    sscanf(argv[argind], "%d", &intVal);
                    if (intVal < 1){
                      Utils::log("The chunk size must be higher than 0\n");
                      return false;
                    }
                    
                    argind++;
                    _chunkSize = intVal;
                  } else {
                    Utils::log("not specify value for the parameter %s\n",
                               argv[argind - 1]);
                    return false;
                  }
                }
	}

	bool allParams = true;

	if(_inputFileName.size() == 0){
		Utils::log("inputFile must be specified\n");
		allParams = false;
	}

	if(_outFileName.size() == 0){
		Utils::log("outFile must be specified\n");
		allParams = false;
	}

	if(_maxVal < 0){
		Utils::log("The maximum value in the discretized dataset must be specified\n");
		allParams = false;
	}

	if(_minRows < 0){
		Utils::log("The minimum number of rows per bicluster must be specified\n");
		allParams = false;
	}

	if(_minColumns < 0){
		Utils::log("The minimum number of columns per bicluster must be specified\n");
		allParams = false;
	}

	return allParams;
}
