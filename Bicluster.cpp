/*
 * Bicluster.cpp
 *
 *  Created on: 15/03/2017
 *      Author: jorge
 */

#include <cassert>
#include <cstdlib>
#include "Bicluster.h"

Bicluster::Bicluster() noexcept :
_numGenes(0),
_numEncodedCols(0),
_allocSizeGenes(0),
_genes(nullptr),
_pattern(nullptr),
_patternCopied(false)
{ }

Bicluster::Bicluster(InputMatrix *mat) :
_numGenes(0),
_numEncodedCols(mat->getNumSamples()/32+1),
_allocSizeGenes(0),
_genes(nullptr),
_pattern(new uint32_t[_numEncodedCols]),
_patternOnes(0),
_patternCopied(true)
{ }

Bicluster::Bicluster(InputMatrix *mat, int gen1, int gen2) :
_numGenes(2),
_numEncodedCols(mat->getNumSamples()/32+1),
_allocSizeGenes(4),
_genes(static_cast<int *>(malloc(sizeof(int) * _allocSizeGenes))),
_pattern(new uint32_t[_numEncodedCols]),
_patternOnes(0),
_patternCopied(true)
{
  _genes[0] = gen1;
  _genes[1] = gen2;
  
  uint32_t * const encoded1 = mat->getEncodedGene(gen1);
  uint32_t * const encoded2 = mat->getEncodedGene(gen2);
  uint32_t aux;
  
  for(int i=0; i<_numEncodedCols; i++){
    aux = encoded1[i] & encoded2[i];
    _pattern[i] = aux;
    _patternOnes += Utils::popcount(aux);
  }
}

Bicluster::Bicluster(uint32_t *pattern, int numSamples, int gen1, int gen2, int patternOnes) :
_numGenes(2),
_numEncodedCols(numSamples/32+1),
_allocSizeGenes(4),
_genes(static_cast<int *>(malloc(sizeof(int) * _allocSizeGenes))),
_pattern(pattern),
_patternOnes(patternOnes),
_patternCopied(false)
{
  _genes[0] = gen1;
  _genes[1] = gen2;
}

/*
Bicluster::Bicluster(uint32_t *pattern, int numSamples, int gen1, int gen2, int patternOnes, int *genes_buf, int genes_buf_size) noexcept :
_numGenes(2),
_numEncodedCols(numSamples/32+1),
_allocSizeGenes(genes_buf_size),
_genes(genes_buf),
_pattern(pattern),
_patternOnes(patternOnes),
_patternCopied(false)
{
  _genes[0] = gen1;
  _genes[1] = gen2;
}
*/

Bicluster::Bicluster(const Bicluster& other) :
_numGenes(other._numGenes),
_numEncodedCols(other._numEncodedCols),
_allocSizeGenes(other._numGenes),  // only allocates for existing genes
_genes(static_cast<int *>(malloc(sizeof(int) * other._numGenes))),
_pattern(other._patternCopied ? new uint32_t[other._numEncodedCols] : other._pattern),
_patternOnes(other._patternOnes),
_patternCopied(other._patternCopied)
{
  memcpy(_genes, other._genes, sizeof(int) * _numGenes);
  if(_patternCopied){
    memcpy(_pattern, other._pattern, sizeof(uint32_t) * _numEncodedCols);
  }
}

Bicluster::Bicluster(Bicluster&& other) noexcept :
_numGenes(other._numGenes),
_numEncodedCols(other._numEncodedCols),
_allocSizeGenes(other._allocSizeGenes),
_genes(other._genes),
_pattern(other._pattern),
_patternOnes(other._patternOnes),
_patternCopied(other._patternCopied)
{
  other._genes = nullptr;
  other._patternCopied = false;
  other._pattern = nullptr;
}

/*
void Bicluster::redefine(InputMatrix *mat, int gen1, int gen2) noexcept
{
  assert(!_numGenes);
  assert(_numEncodedCols == (mat->getNumSamples()/32+1));
  assert(!_allocSizeGenes);
  assert(_genes == nullptr);
  assert(_pattern != nullptr);

  _patternOnes = 0;
  uint32_t * const encoded1 = mat->getEncodedGene(gen1);
  uint32_t * const encoded2 = mat->getEncodedGene(gen2);
  uint32_t * __restrict__ pat = _pattern;
  uint32_t aux;
  
  for(int i=0; i<_numEncodedCols; i++){
    aux = encoded1[i] & encoded2[i];
    pat[i] = aux;
    _patternOnes += Utils::popcount(aux);
  }
}

Bicluster::~Bicluster() {
	if(_genes != nullptr){
		free(_genes);
	}

	if(_patternCopied){
		if(_pattern != nullptr){
			delete [] _pattern;
		}
	}
}

bool Bicluster::insertGene(InputMatrix *mat, int gen){

	if((gen == _genes[0]) || (gen == _genes[1])){
		return false;
	}

	uint32_t *encoded = mat->getEncodedGene(gen);
	uint32_t aux;


	for(int i=0; i<_numEncodedCols; i++){
		aux = _pattern[i] & encoded[i];

		if(aux != _pattern[i]){
			return false;
		}
	}

	_numGenes++;

	if(_allocSizeGenes < (uint32_t) _numGenes){
		_duplicateGeneSize();
	}

	_genes[_numGenes-1] = gen;
	return true;
}

void Bicluster::_duplicateGeneSize(){
  _allocSizeGenes *= 2;
  _genes = static_cast<int *>(realloc(_genes, sizeof(int) * _allocSizeGenes));
}
*/
