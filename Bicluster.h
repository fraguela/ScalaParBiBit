/*
 * Bicluster.h
 *
 *  Created on: 15/03/2017
 *      Author: jorge
 */

#ifndef BICLUSTER_H_
#define BICLUSTER_H_

#include "InputMatrix.h"

class Bicluster {
public:

  Bicluster() noexcept;
  Bicluster(InputMatrix *mat);
  Bicluster(InputMatrix *mat, int gen1, int gen2);
  Bicluster(uint32_t *pattern, int numSamples, int gen1, int gen2, int patterOnes);

  Bicluster(uint32_t *pattern, int numSamples, int gen1, int gen2, int patternOnes, int *genes_buf, int genes_buf_size) noexcept :
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

  Bicluster(const Bicluster& other);
  Bicluster(Bicluster&& other) noexcept;
  
  virtual ~Bicluster()
  {
    if(_genes != nullptr){
      free(_genes);
    }
    
    if(_patternCopied){
      if(_pattern != nullptr){
        delete [] _pattern;
      }
    }
  }
  
  inline int getNumGenes() const noexcept {
    return _numGenes;
  }
  
  inline int *getGenes() const noexcept {
    return _genes;
  }
  
  inline int getGene(int index) const noexcept {
    return _genes[index];
  }
  
  inline uint32_t *getPattern() const noexcept {
    return _pattern;
  }
  
  inline int getPatternOnes() const noexcept {
    return _patternOnes;
  }
  
  inline void reset(int *& genes_buf, int& genes_buf_size) noexcept
  {
    genes_buf = _genes;
    genes_buf_size = _allocSizeGenes;
    _genes = nullptr;
  }
  
  void redefine(InputMatrix *mat, int gen1, int gen2) noexcept
  {
    assert(!_numGenes);
    assert(_numEncodedCols == (mat->getNumSamples()/32+1));
    assert(!_allocSizeGenes);
    assert(_genes == nullptr);
    assert(_pattern != nullptr);
    
    _patternOnes = 0;
    const uint32_t * const encoded1 = mat->getEncodedGene(gen1);
    const uint32_t * const encoded2 = mat->getEncodedGene(gen2);
    uint32_t * __restrict__ pat = _pattern;
    uint32_t aux;
    
    for(int i=0; i<_numEncodedCols; i++){
      aux = encoded1[i] & encoded2[i];
      pat[i] = aux;
      _patternOnes += Utils::popcount(aux);
    }
  }
  
  // It only inserts if the pattern is the same
  bool insertGene(InputMatrix *mat, int gen)
  {
    if((gen == _genes[0]) || (gen == _genes[1])){
      return false;
    }
    
    const uint32_t * const encoded = mat->getEncodedGene(gen);
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
  
private:
  
  void _duplicateGeneSize()
  {
    _allocSizeGenes *= 2;
    _genes = static_cast<int *>(realloc(_genes, sizeof(int) * _allocSizeGenes));
  }
  
  int _numGenes;
  int _numEncodedCols;
  uint32_t _allocSizeGenes;
  
  int *_genes;
  uint32_t *_pattern;
  int _patternOnes;
  
  // To indicate whether the pattern has been copied or it is just a pointer to an external array
  bool _patternCopied;
};

#endif /* BICLUSTER_H_ */
