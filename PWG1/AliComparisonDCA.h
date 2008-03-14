#ifndef ALICOMPARISONDCA_H
#define ALICOMPARISONDCA_H

//------------------------------------------------------------------------------
// Class to keep information from comparison of 
// reconstructed and MC particle tracks (DCA - Distance of Closest Approach 
// to the vertex).   
// 
// Author: J.Otwinowski 04/02/2008 
//------------------------------------------------------------------------------

class TFile;
class AliMCInfo;
class AliESDRecInfo;
class AliESDEvent; 
class AliESD;
class AliESDfriend;
class AliRecInfoCuts;
class AliMCInfoCuts;
class TH1I;
class TH3F;
class TH3;
class TProfile;
class TProfile2D;
class AliESDVertex;

#include "TNamed.h"

class AliComparisonDCA : public TNamed {
public :
  AliComparisonDCA(); 
  ~AliComparisonDCA();
  void      InitHisto();
  void      InitCuts();
  void      Exec(AliMCInfo* infoMC, AliESDRecInfo *infoRC);
  void      Process(AliMCInfo* infoMC, AliESDRecInfo *infoRC);
  
  // Selection cuts
  void SetAliRecInfoCuts(AliRecInfoCuts* cuts=0) {fCutsRC = cuts;}
  void SetAliMCInfoCuts(AliMCInfoCuts* cuts=0) {fCutsMC = cuts;}  

  AliRecInfoCuts*  GetAliRecInfoCuts() const {return fCutsRC;}
  AliMCInfoCuts*   GetAliMCInfoCuts()  const {return fCutsMC;}

  // Merge output objects (needed by PROOF) 
  virtual Long64_t Merge(TCollection* list);

  // Analyse output histograms
  void Analyse();

private:
  // DCA resolution
  TH3F  *fD0TanSPtB1;     //-> distance to vertex y (no ITS clusters) 
  TH3F  *fD1TanSPtB1;     //-> distance to vertex z (no ITS clusters) 
  TH3F  *fD0TanSPtL1;     //-> distance to vertex y  
  TH3F  *fD1TanSPtL1;     //-> distance to vertex z 
  TH3F  *fD0TanSPtInTPC;  //-> distance to vertex y (Inner TPC track parameters) 
  TH3F  *fD1TanSPtInTPC;  //-> distance to vertex z (Inner TPC track parameters)

  AliESDVertex *fVertex;  //! 

  // Global cuts objects
  AliRecInfoCuts*  fCutsRC; // selection cuts for reconstructed tracks
  AliMCInfoCuts*  fCutsMC;  // selection cuts for MC tracks

  AliComparisonDCA(const AliComparisonDCA&); // not implemented
  AliComparisonDCA& operator=(const AliComparisonDCA&); // not implemented

  ClassDef(AliComparisonDCA,1);
};

#endif
