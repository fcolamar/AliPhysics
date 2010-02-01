#ifndef ALIKMEANSCLUSTERING_H
#define ALIKMEANSCLUSTERING_H
 
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

// Implemenatation of the K-Means Clustering Algorithm
// http://en.wikipedia.org/wiki/K-means_clustering
// This particular implementation is the so called Soft K-means algorithm.
// It has been modified to work on the cylindrical topology in eta-phi space.
//
// Author: Andreas Morsch (CERN)
// andreas.morsch@cern.ch

#include <TObject.h>
 
class AliKMeansClustering : public TObject
{
 public:
  AliKMeansClustering()          {}
  virtual ~AliKMeansClustering() {}
  
  static Int_t SoftKMeans (Int_t k, Int_t n, Double_t* x, Double_t* y, Double_t* mx, Double_t* my , Double_t* rk );
  static Int_t SoftKMeans2(Int_t k, Int_t n, Double_t* x, Double_t* y, Double_t* mx, Double_t* my , Double_t* sigma2, 
			  Double_t* rk );
  static Int_t SoftKMeans3(Int_t k, Int_t n, Double_t* x, Double_t* y, Double_t* mx, Double_t* my , 
			   Double_t* sigmax2, Double_t* sigmay2, Double_t* rk );
  static void  OptimalInit(Int_t k, Int_t n, Double_t* x, Double_t* y, Double_t* mx, Double_t* my);
  static void  SetBeta(Double_t beta) {fBeta = beta;}
  static Double_t d(Double_t mx, Double_t my, Double_t x, Double_t y);
  static Int_t NTwoSigma(Int_t k, Int_t n, Double_t* x, Double_t* y, Double_t* mx, Double_t* my, 
				Double_t* sigma2x, Double_t* sigma2y);
protected:
  static Double_t fBeta;
  
  ClassDef(AliKMeansClustering, 1)
};
 
#endif
