/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

////////////////////////////////////////////////////////////////////////////
//
//  This class is used to perform femtoscopic analysis on K0s particles, 
//  which are reconstructed using the AliAODv0 class.  
//
//  authors: Matthew Steinpreis (matthew.steinpreis@cern.ch)
//   
//  Change log:
//	- TOF mismatch function calls changed (4/18/13)
//	- added minimum decay length cut (rarely used though) (3/28/13)
//	- K0 multiplicity histogram now filled with "unskippedCount" instead
//	  of k0Count (which included skipped k0s with shared daughters) 
//	  (3/25/13)
//	- added hists for 3D mom. in LF and PRF (3/28/13) 
//	- changed calling of PIDResponse (should be same actions) (3/28/13)
//	- keep "side" K0s for mass plot (4/18)
//		- tweaked loading and skipping appropriately
//		- use merit test to skip sides (against good and side K0s)
//		- a good K0 cant be skipped by a side
//	- moved TPC propagation (via Hans' method) up to v0 level, which now
//	  uses an AliAODTrack(AliVTrack) instead of AliESDtrack (5/31/13)
//	- added primary vertex subtraction in TPC propagation	(5/31/13)
//	- removed all instances of AliESDtrack usage 	(5/31/13)
//	- removed old separation method/histograms   	(5/31/13)
//	- tidied up LCMS boost			 				(6/10/13)
//	- added new boosting prescription, get q out-side-long for LCMS and PRF (6/24/13)
//		- added histograms and values for LCMS momenta (for simulation)
//	- added random particle order switch in correlations (9/09/13)
//	- added more bins for 3D OSL analysis (9/19/13)
//	- added merit cut choice, pass as argument (10/16/13)
//		- 1-mass, 2-v0dca, 3-dddca, 4-combination (used to be v0dca)
//	- added passable argument for two-track minimum separation (10/16/13)
//	- added boolean to turn off field-sign dependence for train (10/30/13)
////////////////////////////////////////////////////////////////////////////////


 
#include <iostream>
#include <math.h>
#include "TMath.h"
#include "TChain.h"
#include "TFile.h"
#include "TKey.h"
#include "TObject.h"
#include "TObjString.h"
#include "TList.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TProfile.h"
#include "TCanvas.h"
#include "TRandom3.h"

#include "AliAnalysisTask.h"
#include "AliAnalysisManager.h"

#include "AliAODEvent.h"
#include "AliAODInputHandler.h"
#include "AliAODMCParticle.h"
#include "AliAODv0.h"
#include "AliAODRecoDecay.h"
#include "AliCentrality.h"

#include "AliFemtoK0Analysis.h"

#define PI 3.1415927


// Author: Matt Steinpreis, adapted from Dhevan Gangadharan

ClassImp(AliFemtoK0Analysis)

//________________________________________________________________________
AliFemtoK0Analysis::AliFemtoK0Analysis():
AliAnalysisTaskSE(),
  fSignDep(kFALSE),
  fFieldPos(kTRUE),
  fOnlineCase(kTRUE),
  fMeritCase(kTRUE),
  fMinDecayLength(0.0),
  fMeritCutChoice(0),
  fMinSep(0.0),
  fEventCount(0),
  fEC(0x0),
  fEvt(0X0),
  fRandomNumber(0x0),
  fName(0x0),
  fAOD(0x0),
  fOutputList(0x0),
  fPidAOD(0x0)
{
}
//________________________________________________________________________
AliFemtoK0Analysis::AliFemtoK0Analysis(const char *name, bool SignDep, bool FieldPositive, bool OnlineCase, bool MeritCase, float MinDL, int MeritCutChoice, float MinSep) 
: AliAnalysisTaskSE(name),
  fSignDep(SignDep),
  fFieldPos(FieldPositive),
  fOnlineCase(OnlineCase),
  fMeritCase(MeritCase),
  fMinDecayLength(MinDL),
  fMeritCutChoice(MeritCutChoice),
  fMinSep(MinSep),
  fEventCount(0),
  fEC(0x0),
  fEvt(0X0),
  fRandomNumber(0x0),
  fName(name),
  fAOD(0x0),
  fOutputList(0x0),
  fPidAOD(0x0)
{
  //main constructor
  fSignDep		= SignDep;
  fFieldPos 	= FieldPositive;
  fOnlineCase 	= OnlineCase;
  fMeritCase 	= MeritCase;
  fMinDecayLength = MinDL;
  fMeritCutChoice = MeritCutChoice;
  fMinSep 		= MinSep;

  // Define output slots here 
  // Output slot #1
  DefineOutput(1, TList::Class());
  
}
//________________________________________________________________________
AliFemtoK0Analysis::AliFemtoK0Analysis(const AliFemtoK0Analysis &obj)
: AliAnalysisTaskSE(obj.fName),
  fSignDep(obj.fSignDep),
  fFieldPos(obj.fFieldPos),
  fOnlineCase(obj.fOnlineCase),
  fMeritCase(obj.fMeritCase),
  fMinDecayLength(obj.fMinDecayLength),
  fMeritCutChoice(obj.fMeritCutChoice),
  fMinSep(obj.fMinSep),
  fEventCount(obj.fEventCount),
  fEC(obj.fEC),
  fEvt(obj.fEvt),
  fRandomNumber(obj.fRandomNumber),
  fName(obj.fName),
  fAOD(obj.fAOD),
  fOutputList(obj.fOutputList),
  fPidAOD(obj.fPidAOD)
{
}
//________________________________________________________________________
AliFemtoK0Analysis &AliFemtoK0Analysis::operator=(const AliFemtoK0Analysis &obj)
{
 //Assignment operator
 if (this == &obj) return *this;
 
 fSignDep   = obj.fSignDep;
 fFieldPos 	= obj.fFieldPos;
 fOnlineCase 	= obj.fOnlineCase;
 fMeritCase 	= obj.fMeritCase;
 fMinDecayLength= obj.fMinDecayLength;
 fMeritCutChoice= obj.fMeritCutChoice;
 fMinSep		= obj.fMinSep;
 fEventCount 	= obj.fEventCount;
 fEC 		= obj.fEC;
 fEvt 		= obj.fEvt;
 fRandomNumber 	= obj.fRandomNumber;
 fName 		= obj.fName;
 fAOD 		= obj.fAOD;
 fOutputList 	= obj.fOutputList;
 fPidAOD 	= obj.fPidAOD;

 return (*this);
}
//________________________________________________________________________
AliFemtoK0Analysis::~AliFemtoK0Analysis()
{
  // Destructor
  if(fEC) delete fEC;
  if(fEvt) delete fEvt;
  if(fRandomNumber) delete fRandomNumber;
  if(fName) delete fName;
  if(fAOD) delete fAOD;
  if(fOutputList) delete fOutputList;
  if(fPidAOD) delete fPidAOD;
}
//________________________________________________________________________
void AliFemtoK0Analysis::MyInit()
{

  // One can set global variables here
  fEventCount = 0;  

  fEC = new AliFemtoK0EventCollection **[kZVertexBins];
  for(unsigned short i=0; i<kZVertexBins; i++)
  {
    fEC[i] = new AliFemtoK0EventCollection *[kCentBins];
    
    for(unsigned short j=0; j<kCentBins; j++)
    {
      fEC[i][j] = new AliFemtoK0EventCollection(kEventsToMix+1, kMultLimit);
    }
  }

  AliAODInputHandler *aodH = dynamic_cast<AliAODInputHandler*>(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler());
  fPidAOD = aodH->GetPIDResponse();

  fRandomNumber = new TRandom3();  //for 3D, random sign switching
  fRandomNumber->SetSeed(0);

}
//________________________________________________________________________
void AliFemtoK0Analysis::UserCreateOutputObjects()
{
  // Create histograms
  // Called once
  
  MyInit();// Initialize my settings

  fOutputList = new TList();
  fOutputList->SetOwner();

  TH1F *fHistCent = new TH1F("fHistCent","",100,0,100);
  fOutputList->Add(fHistCent);
 
  //pion parameters
  TH1F* fHistDCAPiPlus = new TH1F("fHistDCAPiPlus","",100,0,10);
  fOutputList->Add(fHistDCAPiPlus);
  TH1F* fHistDCAPiMinus = new TH1F("fHistDCAPiMinus","",100,0,10);
  fOutputList->Add(fHistDCAPiMinus);
  TH1F* fHistDCADaughters = new TH1F("fHistDCADaughters", "DCA of pions to each other", 50, 0., 0.5);
  fOutputList->Add(fHistDCADaughters);
  TH2F* fHistK0PiPlusPt = new TH2F("fHistK0PiPlusPt", "", kCentBins, .5,kCentBins+.5, 40,0.,4.);
  fOutputList->Add(fHistK0PiPlusPt);
  TH2F* fHistK0PiMinusPt = new TH2F("fHistK0PiMinusPt", "", kCentBins, .5,kCentBins+.5, 40,0.,4.);
  fOutputList->Add(fHistK0PiMinusPt);
  TH1F* fHistDaughterPhi = new TH1F("fHistDaughterPhi","",180,-PI,PI);
  fOutputList->Add(fHistDaughterPhi);

  //K0 parameters
  TH1F* fHistMultK0 = new TH1F("fHistMultK0", "K0 multiplicity", 51, -0.5, 51-0.5);
  fOutputList->Add(fHistMultK0);
  TH2F* fHistPtK0 = new TH2F("fHistPtK0", "K0 pt distribution",kCentBins,.5,kCentBins+.5, 100, 0., 10.);
  fOutputList->Add(fHistPtK0);
  TH1F* fHistDecayLengthK0 = new TH1F("fHistDecayLengthK0", "K0 decay length", 100, 0., 100.);
  fOutputList->Add(fHistDecayLengthK0);
  TH1F* fHistDCAK0 = new TH1F("fHistDCAK0", "DCA of K0 to primary vertex", 40, 0., 0.4);
  fOutputList->Add(fHistDCAK0);
  TH2F* fHistKtK0 = new TH2F("fHistKtK0", "Kt distribution of K0 pairs", kCentBins, .5, kCentBins+.5, 300, 0., 3.);
  fOutputList->Add(fHistKtK0);

  TH1F* fHistPx = new TH1F("fHistPx","",200,0,2);
  TH1F* fHistPy = new TH1F("fHistPy","",200,0,2);
  TH1F* fHistPz = new TH1F("fHistPz","",200,0,2);
  TH1F* fHistPxCM = new TH1F("fHistPxCM","",200,0,2);
  TH1F* fHistPyCM = new TH1F("fHistPyCM","",200,0,2);
  TH1F* fHistPzCM = new TH1F("fHistPzCM","",200,0,2);
  TH1F* fHistKsCM = new TH1F("fHistKsCM","",200,0,2);
  fOutputList->Add(fHistPx);
  fOutputList->Add(fHistPy);
  fOutputList->Add(fHistPz);
  fOutputList->Add(fHistPxCM);
  fOutputList->Add(fHistPyCM);
  fOutputList->Add(fHistPzCM);
  fOutputList->Add(fHistKsCM);

  TH1F* fHistPOutLCMS = new TH1F("fHistPOutLCMS","",200,0,2);
  TH1F* fHistPSideLCMS = new TH1F("fHistPSideLCMS","",200,0,2);
  TH1F* fHistPLongLCMS = new TH1F("fHistPLongLCMS","",200,0,2);
  fOutputList->Add(fHistPOutLCMS);
  fOutputList->Add(fHistPSideLCMS);
  fOutputList->Add(fHistPLongLCMS);

  //pair gamma (LCMS to PRF, OSL)
  TH2F* fHistGamma = new TH2F("fHistGamma","Gamma from LCMS to PRF",500,1,5,100,0,1);
  fOutputList->Add(fHistGamma);

  //invariant mass distributions
  TH3F* fHistMass = new TH3F("fHistMass","",kCentBins,.5,kCentBins+.5,50,0.,5.,400,.3,.7);
  fOutputList->Add(fHistMass);
  //TH3F* fHistMassPtCFK0 = new TH3F("fHistMassPtCFK0","",kCentBins,.5,kCentBins+.5,50,0.,5.,200,.4,.6);
  //fOutputList->Add(fHistMassPtCFK0);
  //TH3F* fHistMassPtCFBkgK0 = new TH3F("fHistMassPtCFBkgK0","",kCentBins,.5,kCentBins+.5,50,0.,5.,200,.4,.6);
  //fOutputList->Add(fHistMassPtCFBkgK0);
  //TH3F* fHistMassQKt = new TH3F("fHistMassQKt","",100,0,1,200,0,2,200,.4,.6);
  //fOutputList->Add(fHistMassQKt);
  //TH3F* fHistMassKtK0 = new TH3F("fHistMassKtK0","",kCentBins,.5,kCentBins+.5,300,0.,3.,200,.4,.6);
  //fOutputList->Add(fHistMassKtK0);
  //TH3F* fHistMassKtBkgK0 = new TH3F("fHistMassKtBkgK0","",kCentBins,.5,kCentBins+.5,300,0.,3.,200,.4,.6);
  //fOutputList->Add(fHistMassKtBkgK0);

  //separation studies
  TH1F* fHistSepNumPos = new TH1F("fHistSepNumPos","",200,0,20);
  fOutputList->Add(fHistSepNumPos);
  TH1F* fHistSepDenPos = new TH1F("fHistSepDenPos","",200,0,20);
  fOutputList->Add(fHistSepDenPos);
  TH1F* fHistSepNumNeg = new TH1F("fHistSepNumNeg","",200,0,20);
  fOutputList->Add(fHistSepNumNeg);
  TH1F* fHistSepDenNeg = new TH1F("fHistSepDenNeg","",200,0,20);
  fOutputList->Add(fHistSepDenNeg);
  
  TH2F* fHistSepNumPos2 = new TH2F("fHistSepNumPos2","",100,0,20,100,0,20);
  TH2F* fHistSepDenPos2 = new TH2F("fHistSepDenPos2","",100,0,20,100,0,20);
  TH2F* fHistSepNumNeg2 = new TH2F("fHistSepNumNeg2","",100,0,20,100,0,20);
  TH2F* fHistSepDenNeg2 = new TH2F("fHistSepDenNeg2","",100,0,20,100,0,20);
  fOutputList->Add(fHistSepNumPos2);
  fOutputList->Add(fHistSepDenPos2);
  fOutputList->Add(fHistSepNumNeg2);
  fOutputList->Add(fHistSepDenNeg2);


  TH2F* fHistSepDPC = new TH2F("fHistSepDPC","",200,-1,1,50,0,10);
  TH2F* fHistSepDPCBkg = new TH2F("fHistSepDPCBkg","",200,-1,1,50,0,10);
  fOutputList->Add(fHistSepDPC);
  fOutputList->Add(fHistSepDPCBkg);  

/////////Signal Distributions///////////////////

  //1D Q invariant
  TH3F* fHistQinvSignal = new TH3F("fHistQinvSignal","Same Event Pair Distribution", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  fOutputList->Add(fHistQinvSignal);
  TH3F* fHistQinvBkg = new TH3F("fHistQinvBkg","Mixed Event Pair Distribution", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1.);
  fOutputList->Add(fHistQinvBkg);

  //mass bins within peak
  //TH3F* fHistCLCLSignal = new TH3F("fHistCLCLSignal","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistCLCLBkg = new TH3F("fHistCLCLBkg","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistCLCRSignal = new TH3F("fHistCLCRSignal","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistCLCRBkg = new TH3F("fHistCLCRBkg","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistCRCRSignal = new TH3F("fHistCRCRSignal","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistCRCRBkg = new TH3F("fHistCRCRBkg","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //fOutputList->Add(fHistCLCLSignal);
  //fOutputList->Add(fHistCLCLBkg);
  //fOutputList->Add(fHistCLCRSignal);
  //fOutputList->Add(fHistCLCRBkg);
  //fOutputList->Add(fHistCRCRSignal);
  //fOutputList->Add(fHistCRCRBkg);
  

  //3D out-side-long
  /*TH3F* fHistOSLCentLowKt = new TH3F("fHistOSLCentLowKt","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLCentLowKt);
  TH3F* fHistOSLCentLowKtBkg = new TH3F("fHistOSLCentLowKtBkg","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLCentLowKtBkg);

  TH3F* fHistOSLCentHighKt = new TH3F("fHistOSLCentHighKt","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLCentHighKt);
  TH3F* fHistOSLCentHighKtBkg = new TH3F("fHistOSLCentHighKtBkg","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLCentHighKtBkg);

  TH3F* fHistOSLSemiCentLowKt = new TH3F("fHistOSLSemiCentLowKt","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLSemiCentLowKt);
  TH3F* fHistOSLSemiCentLowKtBkg = new TH3F("fHistOSLSemiCentLowKtBkg","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLSemiCentLowKtBkg);

  TH3F* fHistOSLSemiCentHighKt = new TH3F("fHistOSLSemiCentHighKt","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLSemiCentHighKt);
  TH3F* fHistOSLSemiCentHighKtBkg = new TH3F("fHistOSLSemiCentHighKtBkg","",100,-.5,.5,100,-.5,.5,100,-.5,.5);
  fOutputList->Add(fHistOSLSemiCentHighKtBkg);*/

  //3D out-side-long
  TH3F *fHist3DOSLSignal[10][4];
  TH3F *fHist3DOSLBkg[10][4];
  
  for(int i3D=0;i3D<10;i3D++){
   for(int j3D=0;j3D<4;j3D++){
    TString *histname = new TString("fHist3DOSL");
    *histname += i3D;
    *histname += j3D;
    histname->Append("Signal");
    fHist3DOSLSignal[i3D][j3D] = new TH3F(histname->Data(),"",100,-.5,.5,100,-.5,.5,100,-.5,.5);
    fOutputList->Add(fHist3DOSLSignal[i3D][j3D]);
    histname->Replace(12,6,"Bkg");
    fHist3DOSLBkg[i3D][j3D] = new TH3F(histname->Data(),"",100,-.5,.5,100,-.5,.5,100,-.5,.5);
    fOutputList->Add(fHist3DOSLBkg[i3D][j3D]);
   }
  }
    


  //3D Spherical Harmonics
  TH3F* fHistSHCentLowKt = new TH3F("fHistSHCentLowKt","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHCentHighKt = new TH3F("fHistSHCentHighKt","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHSemiCentLowKt = new TH3F("fHistSHSemiCentLowKt","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHSemiCentHighKt = new TH3F("fHistSHSemiCentHighKt","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHCentLowKtBkg = new TH3F("fHistSHCentLowKtBkg","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHCentHighKtBkg = new TH3F("fHistSHCentHighKtBkg","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHSemiCentLowKtBkg = new TH3F("fHistSHSemiCentLowKtBkg","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  TH3F* fHistSHSemiCentHighKtBkg = new TH3F("fHistSHSemiCentHighKtBkg","",50,0,.5,ncthetabins,-1,1,nphibins,0,2*PI);
  fOutputList->Add(fHistSHCentLowKt);
  fOutputList->Add(fHistSHCentHighKt);
  fOutputList->Add(fHistSHSemiCentLowKt);
  fOutputList->Add(fHistSHSemiCentHighKt);
  fOutputList->Add(fHistSHCentLowKtBkg);
  fOutputList->Add(fHistSHCentHighKtBkg);
  fOutputList->Add(fHistSHSemiCentLowKtBkg);
  fOutputList->Add(fHistSHSemiCentHighKtBkg);

  //side-side
  //TH3F* fHistLeftLeftSignal = new TH3F("fHistLeftLeftSignal","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistLeftRightSignal = new TH3F("fHistLeftRightSignal","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistRightRightSignal = new TH3F("fHistRightRightSignal","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistLeftLeftBkg = new TH3F("fHistLeftLeftBkg","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistLeftRightBkg = new TH3F("fHistLeftRightBkg","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //TH3F* fHistRightRightBkg = new TH3F("fHistRightRightBkg","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //fOutputList->Add(fHistLeftLeftSignal);
  //fOutputList->Add(fHistLeftRightSignal);
  //fOutputList->Add(fHistRightRightSignal);
  //fOutputList->Add(fHistLeftLeftBkg);
  //fOutputList->Add(fHistLeftRightBkg);
  //fOutputList->Add(fHistRightRightBkg);

  //TH3F* fHistSplitK0Sides = new TH3F("fHistSplitK0Sides","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //fOutputList->Add(fHistSplitK0Sides);
  //TH3F* fHistSplitK0Centers = new TH3F("fHistSplitK0Centers","", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //fOutputList->Add(fHistSplitK0Centers);
  //TH3F* fHistQinvSignalNoSplit = new TH3F("fHistQinvSignalNoSplit","Same Event Pair Distribution", kCentBins, .5, kCentBins+.5, 300, 0., 3., 100, 0., 1);
  //fOutputList->Add(fHistQinvSignalNoSplit);

  PostData(1, fOutputList);

}

//________________________________________________________________________
void AliFemtoK0Analysis::Exec(Option_t *) 
{
  // Main loop
  // Called for each event
  //cout<<"===========  Event # "<<fEventCount+1<<"  ==========="<<endl;
  fEventCount++;
  fAOD = dynamic_cast<AliAODEvent*> (InputEvent());
  if (!fAOD) {Printf("ERROR: fAOD not available"); return;}

  Bool_t isSelected = (((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->IsEventSelected() & (AliVEvent::kMB | AliVEvent::kCentral | AliVEvent::kSemiCentral));
  bool isCentral = (((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->IsEventSelected() & AliVEvent::kCentral);
  //Bool_t isSelected = (((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->IsEventSelected() & AliVEvent::kMB);
  if(!isSelected) {
   //cout << "Failed trigger selection." << endl; 
   return;
  }
  
  ///////////////////////////////////////////////////////////

  unsigned int statusPos=0;
  unsigned int statusNeg=0;

  float bField=0;
  bField = fAOD->GetMagneticField();
  if(bField == 0) return;
  if(fSignDep){
   if(fFieldPos && bField < 0) return;
   if(!fFieldPos && bField > 0) return;
  }

  
  int zBin=0;
  double zStep=2*10/double(kZVertexBins), zstart=-10.;

  /////////////////////////////////////////////////


  //Centrality selection

  AliCentrality *centrality = fAOD->GetCentrality();
  float percent = centrality->GetCentralityPercentile("V0M");
  int centBin=0;
  //Printf("Centrality percent = %f", percent);
  
  AliAODVZERO *aodV0 = fAOD->GetVZEROData();
  float multV0A=aodV0->GetMTotV0A();
  float multV0C=aodV0->GetMTotV0C();

  if(percent < 0) {
   //Printf("No centrality info"); 
   return;
  }
  if(percent < 0.1 && (multV0A + multV0C < 19500)){
   //Printf("No centrality info"); 
   return;
  }
  else if(percent <= 5)   centBin=15;
  else if(percent <= 10)  centBin=14;
  else if(percent <= 15)  centBin=13;
  else if(percent <= 20)  centBin=12;
  else if(percent <= 25)  centBin=11;
  else if(percent <= 30)  centBin=10;
  else if(percent <= 35)  centBin=9;
  else if(percent <= 40)  centBin=8;
  else if(percent <= 45)  centBin=7;
  else if(percent <= 50)  centBin=6;
  else if(percent <= 55)  centBin=5;
  else if(percent <= 60)  centBin=4;
  else if(percent <= 65)  centBin=3;
  else if(percent <= 70)  centBin=2;
  else if(percent <= 75)  centBin=1;
  else if(percent <= 80)  centBin=0;
  else {
   //Printf("Skipping Peripheral Event"); 
   return;
  }
  if(percent > 10 && isCentral) return;
  ((TH1F*)fOutputList->FindObject("fHistCent"))->Fill(percent);
  
  //Vertexing
  AliAODVertex *primaryVertex;
  double vertex[3]={0};
  primaryVertex = fAOD->GetPrimaryVertex();
  vertex[0]=primaryVertex->GetX(); 
  vertex[1]=primaryVertex->GetY(); 
  vertex[2]=primaryVertex->GetZ();
  if(vertex[0]<10e-5 && vertex[1]<10e-5 &&  vertex[2]<10e-5) return;
  if(fabs(vertex[2]) > 10) return; // Z-vertex Cut

  for(int i=0; i<kZVertexBins; i++)
  {
    if((vertex[2] > zstart+i*zStep) && (vertex[2] < zstart+(i+1)*zStep))
    {
      zBin=i;
      break;
    }
  }

////////////////////////////////////////////////////////////////
//Cut Values and constants

  //const bool kMCCase = kFALSE;                     //switch for MC analysis
  const int kMaxNumK0 = 300;                       //maximum number of K0s, array size
  const float kMinDCAPrimaryPion = 0.4;            //minimum dca of pions to primary
  const float kMaxDCADaughtersK0 = 0.3;            //maximum dca of pions to each other - 3D
  const float kMaxDCAK0 = 0.3;                     //maximum dca of K0 to primary
  const float kMaxDLK0 = 30.0;                     //maximum decay length of K0
  const float kMinDLK0 = fMinDecayLength;	   //minimum decay length of K0
  const float kEtaCut = 0.8;                       //maximum |pseudorapidity|
  const float kMinCosAngle = 0.99;                 //minimum cosine of K0 pointing angle     
  
  const float kMinSeparation = fMinSep;                //minimum daughter (pair) separation
                 
  const float kTOFLow = 0.8;                       //boundary for TOF usage
  const float kMaxTOFSigmaPion = 3.0;              //TOF # of sigmas
  const float kMaxTPCSigmaPion = 3.0;              //TPC # of sigmas

  //const float kMassPion = .13957;
  const float kMassK0Short = .497614;       //true PDG masses

////////////////////////////////////////////////////////////////  
  //v0 tester
////////////////////////////////////////////////////////////////
  int v0Count = 0;	//number of v0s (entries in array)
  int k0Count = 0;	//number of good K0s

  AliFemtoK0Particle *tempK0 = new AliFemtoK0Particle[kMultLimit];

  //for daughter sharing studies
  //int idArray[100] = {0};
  //int idCount = 0;

  //for MC
  //TClonesArray *mcArray = 0x0;
  //if(kMCCase){
  //mcArray = (TClonesArray*)fAOD->FindListObject(AliAODMCParticle::StdBranchName());
  //if(!mcArray){cout<<"No MC particle branch found"<<endl;return;}}

  for(int i = 0; i < fAOD->GetNumberOfV0s(); i++)
  {
    bool goodK0 = kFALSE;
    bool goodPiPlus = kFALSE;
    bool goodPiMinus = kFALSE;
    
    //load v0 track
    AliAODv0* v0 = fAOD->GetV0(i);
    if(!v0) continue;
    if(fOnlineCase){
     if(!(v0->GetOnFlyStatus())) continue;
    } //for online
    else{
     if((v0->GetOnFlyStatus())) continue; //for offline
    }
 
    //for on-the-fly ordering
    AliAODTrack* tempTrack = (AliAODTrack*)v0->GetDaughter(0);
    short int pos0or1;
    short int neg0or1;
    bool orderswitch = kFALSE;
    if(tempTrack->Charge() > 0) {pos0or1 = 0; neg0or1 = 1;}
    else {pos0or1 = 1; neg0or1 = 0; orderswitch = kTRUE;}
    //tempTrack->~AliAODTrack();

    //load daughter tracks
    AliAODTrack* prongTrackPos = (AliAODTrack*)v0->GetDaughter(pos0or1);
    AliAODTrack* prongTrackNeg = (AliAODTrack*)v0->GetDaughter(neg0or1);
    if(!prongTrackPos) continue;
    if(!prongTrackNeg) continue;

    //daughter cuts
    if(v0->PtProng(pos0or1) < .15) continue;
    if(v0->PtProng(neg0or1) < .15) continue;
    if(fabs(v0->EtaProng(pos0or1)) > .8) continue;
    if(fabs(v0->EtaProng(neg0or1)) > .8) continue;

    //load status for PID
    statusPos=prongTrackPos->GetStatus();
    if((statusPos&AliESDtrack::kTPCrefit)==0) continue;
    prongTrackPos->SetAODEvent(fAOD);
    statusNeg=prongTrackNeg->GetStatus();
    if((statusNeg&AliESDtrack::kTPCrefit)==0) continue;
    prongTrackNeg->SetAODEvent(fAOD);

    //TPC PID
    if(fabs(fPidAOD->NumberOfSigmasTPC(prongTrackPos,AliPID::kPion)) < kMaxTPCSigmaPion) goodPiPlus = kTRUE;
    if(fabs(fPidAOD->NumberOfSigmasTPC(prongTrackNeg,AliPID::kPion)) < kMaxTPCSigmaPion) goodPiMinus = kTRUE;
   
    //Positive daughter identification TOF
    float probMis;
    AliPIDResponse::EDetPidStatus statusPosTOF = fPidAOD->CheckPIDStatus(AliPIDResponse::kTOF, prongTrackPos);
    double Ppos = v0->PProng(pos0or1);
    if(Ppos > kTOFLow) //PiPlus
    {
     //if( (statusPos&AliESDtrack::kTOFpid)!=0 && (statusPos&AliESDtrack::kTIME)!=0 && (statusPos&AliESDtrack::kTOFout)!=0 && (statusPos&AliESDtrack::kTOFmismatch)<=0) (OBSOLETE; NEW CALL BELOW)
     if(AliPIDResponse::kDetPidOk == statusPosTOF)
     {
      probMis = fPidAOD->GetTOFMismatchProbability(prongTrackPos);
      if(probMis < 0.01) //avoid TOF-TPC mismatch
      {
       if(fabs(fPidAOD->NumberOfSigmasTOF(prongTrackPos,AliPID::kPion)) < kMaxTOFSigmaPion) goodPiPlus = kTRUE;
       else goodPiPlus = kFALSE;
      }  
     }
    }
    //Negative daughter identification TOF
    AliPIDResponse::EDetPidStatus statusNegTOF = fPidAOD->CheckPIDStatus(AliPIDResponse::kTOF, prongTrackNeg);
    double Pneg = v0->PProng(neg0or1);
    if(Pneg > kTOFLow) //PiMinus
    {
     //if( (statusNeg&AliESDtrack::kTOFpid)!=0 && (statusNeg&AliESDtrack::kTIME)!=0 && (statusNeg&AliESDtrack::kTOFout)!=0 && (statusNeg&AliESDtrack::kTOFmismatch)<=0) (OBSOLETE; NEW CALL BELOW)
     if(AliPIDResponse::kDetPidOk == statusNegTOF)
     {
      probMis = fPidAOD->GetTOFMismatchProbability(prongTrackPos);
      if(probMis < 0.01) //avoid TOF-TPC mismatch
      {
       if(fabs(fPidAOD->NumberOfSigmasTOF(prongTrackNeg,AliPID::kPion)) < kMaxTOFSigmaPion) goodPiMinus = kTRUE;
       else goodPiMinus = kFALSE;
      }
     }
    }
    
    //K0 cuts
    if(v0->Eta() > kEtaCut)                                	continue;    
    if(v0->CosPointingAngle(primaryVertex) < kMinCosAngle) 	continue;
    if(v0->MassK0Short() < .2 || v0->MassK0Short() > .8)   	continue;
    if(v0->DcaNegToPrimVertex() < kMinDCAPrimaryPion)      	continue;
    if(v0->DcaPosToPrimVertex() < kMinDCAPrimaryPion)      	continue;  
    if(v0->DecayLength(primaryVertex) > kMaxDLK0)          	continue;
    if(v0->DecayLength(primaryVertex) < kMinDLK0)	   		continue;
    if(v0->DcaV0Daughters() > kMaxDCADaughtersK0)          	continue;
    double v0Dca = v0->DcaV0ToPrimVertex();
    if(v0Dca > kMaxDCAK0) 		                   			continue;        
    if(!goodPiMinus || !goodPiPlus)                        	continue; 
    
    //EVERYTHING BELOW HERE PASSES SINGLE PARTICLE CUTS, PION PID, and LOOSE MASS CUT

    //for MC
    //bool MCgood = kFALSE;
    //if(kMCCase){
    //AliAODMCParticle* mck0dp = (AliAODMCParticle*)mcArray->At(abs(prongTrackPos->GetLabel()));
    //AliAODMCParticle* mck0dn = (AliAODMCParticle*)mcArray->At(abs(prongTrackNeg->GetLabel()));   
    //if(mck0dp->GetMother() >= 0){ 
     //if(mck0dp->GetMother() == mck0dn->GetMother()){
      //if(abs(mck0dp->GetPdgCode()) == 211 && abs(mck0dn->GetPdgCode()) == 211){
       //AliAODMCParticle* mck0 = (AliAODMCParticle*)mcArray->At(mck0dp->GetMother());
       //if(abs(mck0->GetPdgCode()) == 310){
        //MCgood = kTRUE;     
       //}
      //}
     //}
    //}
    //}// if kMCCase
    
    if(v0->MassK0Short() > .48 && v0->MassK0Short() < .515) goodK0 = kTRUE;
    //else continue; //removed, Apr 18
     
    //Check for shared daughters, using v0 DCA to judge
    bool v0JudgeNew; //true if new v0 beats old
    tempK0[v0Count].fSkipShared = kFALSE;
    double newV0Pars[3] = {fabs(v0->MassK0Short()-kMassK0Short),v0Dca,v0->DcaV0Daughters()}; //parameters used in merit cut
    if(fMeritCase){
     for(int iID = 0; iID<v0Count; iID++){
      if(tempK0[iID].fSkipShared == kFALSE){		//if old is already skipped, go to next old
       if(tempK0[iID].fDaughterID1 == prongTrackPos->GetID() || tempK0[iID].fDaughterID2 == prongTrackNeg->GetID()){
        double oldV0Pars[3] = {fabs(tempK0[iID].fMass-kMassK0Short), tempK0[iID].fV0Dca, tempK0[iID].fDDDca}; 
        v0JudgeNew = CheckMeritCutWinner(fMeritCutChoice, oldV0Pars, newV0Pars); //true if new wins
        if(!v0JudgeNew){		//if old beats new...
         if(!tempK0[iID].fK0 && goodK0) continue;	//if bad old beats new good, do nothing...				
         else{						//but if bad old beats new bad, or good old beats anything, skip new
          tempK0[v0Count].fSkipShared = kTRUE;		//skip new
          break;					//no need to keep checking others
         }
        }
        else{						//if new beats old...
         if(tempK0[iID].fK0 && !goodK0) continue;	//if bad new beats good old, do nothing...
         else{						//but if bad new beats bad old, or good new beats anything, skip old
	      tempK0[iID].fSkipShared = kTRUE;		//skip old	
	      if(tempK0[iID].fK0) k0Count--;		//if good old gets skipped, subtract from number of K0s (new one will be added later, if it succeeds)
         }
        }
       }
      }
     }
     if(tempK0[v0Count].fSkipShared) continue;		//if new K0 is skipped, don't load; go to next v0
    }//if MeritCase	 		
	
    //load parameters into temporary class instance
    if(v0Count < kMaxNumK0)
    {
	if(goodK0){
         tempK0[v0Count].fK0 = kTRUE;
         k0Count++;
        }
        else tempK0[v0Count].fK0 = kFALSE; 

        //if(v0->MassK0Short() > .45 && v0->MassK0Short() < .48) tempK0[v0Count].fSideLeft = kTRUE;
        //else tempK0[v0Count].fSideLeft = kFALSE;
        //if(v0->MassK0Short() > .515 && v0->MassK0Short() < .545) tempK0[v0Count].fSideRight = kTRUE;
        //else tempK0[v0Count].fSideRight = kFALSE;
	    //if(!goodK0) continue; //no sides, speed up analysis (REDUNDANT RIGHT NOW)

        tempK0[v0Count].fDaughterID1    = prongTrackPos->GetID();
        tempK0[v0Count].fDaughterID2    = prongTrackNeg->GetID();
        tempK0[v0Count].fMomentum[0]   	= v0->Px();
        tempK0[v0Count].fMomentum[1]   	= v0->Py();
        tempK0[v0Count].fMomentum[2]   	= v0->Pz();
        tempK0[v0Count].fPt       	= v0->Pt();
        tempK0[v0Count].fMass   	= v0->MassK0Short();
        tempK0[v0Count].fV0Dca		= v0Dca;

        //for hists
        tempK0[v0Count].fDDDca		= v0->DcaV0Daughters();
	    tempK0[v0Count].fDecayLength	= v0->DecayLength(primaryVertex);
        tempK0[v0Count].fPosPt		= v0->PtProng(pos0or1);
        tempK0[v0Count].fNegPt		= v0->PtProng(neg0or1);
        tempK0[v0Count].fPosPhi		= v0->PhiProng(pos0or1);
        tempK0[v0Count].fNegPhi		= v0->PhiProng(neg0or1);
	    if(!orderswitch){
         tempK0[v0Count].fPosDca	= v0->DcaPosToPrimVertex();
         tempK0[v0Count].fNegDca	= v0->DcaNegToPrimVertex();
	    }
        else{
         tempK0[v0Count].fPosDca	= v0->DcaNegToPrimVertex();
         tempK0[v0Count].fNegDca	= v0->DcaPosToPrimVertex();
	    } 
        
        //for separation
        GetGlobalPositionAtGlobalRadiiThroughTPC(prongTrackPos, bField, tempK0[v0Count].fPosXYZ, vertex);
	    GetGlobalPositionAtGlobalRadiiThroughTPC(prongTrackNeg, bField, tempK0[v0Count].fNegXYZ, vertex);
        //for DPC
        prongTrackPos->GetPxPyPz(tempK0[v0Count].fPPos);
        prongTrackNeg->GetPxPyPz(tempK0[v0Count].fPNeg);

        //if(idCount < 50){
        // if(goodK0){
        //  idArray[idCount*2]   = prongTrackPos->GetID();
        //  idArray[idCount*2+1] = prongTrackNeg->GetID();
        //  idCount++;
        //}} 

        v0Count++;
    }

    v0->~AliAODv0();
    }//v0
   
  if(k0Count<2) return;  //only keep events with more than 1 good K0

  //Add Event to buffer - this is for event mixing
  fEC[zBin][centBin]->FIFOShift();
  (fEvt) = fEC[zBin][centBin]->fEvt;
  (fEvt)->fFillStatus = 1;
  int unskippedCount = 0;
  for(int i=0;i<v0Count;i++)
  {
   if(!tempK0[i].fSkipShared)				//don't include skipped v0s (from shared daughters)
   {
    ((TH3F*)fOutputList->FindObject("fHistMass"))->Fill(centBin+1,tempK0[i].fPt,tempK0[i].fMass);
    if(tempK0[i].fK0)					//make sure particle is good (mass)
    {
     (fEvt)->fK0Particle[unskippedCount] = tempK0[i];	//load good, unskipped K0s
     unskippedCount++;					//count good, unskipped K0s
    }
   }
  }
  (fEvt)->fNumV0s = unskippedCount;
  //Printf("Number of v0s: %d", v0Count);
  //Printf("Number of K0s: %d", k0Count);
  tempK0->~AliFemtoK0Particle();

  ((TH1F*)fOutputList->FindObject("fHistMultK0"))->Fill(unskippedCount);	// changed 3/25, used to be "k0Count"

  //Printf("Reconstruction Finished. Starting pair studies.");

  //////////////////////////////////////////////////////////////////////
  // Correlations
  //////////////////////////////////////////////////////////////////////

  float px1, py1, pz1, px2, py2, pz2;			//single kaon values
  float en1, en2;								//single kaon values 
  //float pt1, pt2; 								//single kaon values
  float pairPx, pairPy, pairPz, pairP0;			//pair momentum values
  float pairPt, pairMt, pairKt;         		//pair momentum values
  float pairMInv, pairPDotQ;
  float qinv, q0, qx, qy, qz;      			//pair q values
  //float qLength, thetaSH, thetaSHCos, phiSH;            //Spherical Harmonics values
  float am12, epm, h1, p12, p112, ppx, ppy, ppz, ks;	//PRF
  //float qOutLCMS;
  float qOutPRF, qSide, qLong;				//relative momentum in LCMS/PRF frame
  float betasq, gamma;
  float p1LCMSOut, p1LCMSSide, p1LCMSLong, en1LCMS;
  float p2LCMSOut, p2LCMSSide, p2LCMSLong, en2LCMS;


  for(int i=0; i<(fEvt)->fNumV0s; i++) // Current event V0
  {
    //single particle histograms (done here to avoid "skipped" v0s
     ((TH1F*)fOutputList->FindObject("fHistDCADaughters"))	->Fill((fEvt)->fK0Particle[i].fDDDca);
     ((TH1F*)fOutputList->FindObject("fHistDecayLengthK0"))	->Fill((fEvt)->fK0Particle[i].fDecayLength);
     ((TH1F*)fOutputList->FindObject("fHistDCAK0"))		->Fill((fEvt)->fK0Particle[i].fV0Dca);
     ((TH1F*)fOutputList->FindObject("fHistDCAPiMinus"))	->Fill((fEvt)->fK0Particle[i].fNegDca);
     ((TH1F*)fOutputList->FindObject("fHistDCAPiPlus"))		->Fill((fEvt)->fK0Particle[i].fPosDca);
     ((TH2F*)fOutputList->FindObject("fHistPtK0"))		->Fill(centBin+1, (fEvt)->fK0Particle[i].fPt);
     ((TH2F*)fOutputList->FindObject("fHistK0PiPlusPt"))	->Fill(centBin+1, (fEvt)->fK0Particle[i].fPosPt);
     ((TH2F*)fOutputList->FindObject("fHistK0PiMinusPt"))	->Fill(centBin+1, (fEvt)->fK0Particle[i].fNegPt);
     ((TH1F*)fOutputList->FindObject("fHistDaughterPhi"))	->Fill((fEvt)->fK0Particle[i].fPosPhi);
     ((TH1F*)fOutputList->FindObject("fHistDaughterPhi"))	->Fill((fEvt)->fK0Particle[i].fNegPhi);
     
     ((TH1F*)fOutputList->FindObject("fHistPx"))		->Fill((fEvt)->fK0Particle[i].fMomentum[0]);
     ((TH1F*)fOutputList->FindObject("fHistPy"))		->Fill((fEvt)->fK0Particle[i].fMomentum[1]);
     ((TH1F*)fOutputList->FindObject("fHistPz"))		->Fill((fEvt)->fK0Particle[i].fMomentum[2]);

    for(int evnum=0; evnum<kEventsToMix+1; evnum++)// Event buffer loop: evnum=0 is the current event, all other evnum's are past events
    {
      int startbin=0;
      if(evnum==0) startbin=i+1;
      
      for(int j=startbin; j<(fEvt+evnum)->fNumV0s; j++) // Past event V0
      {
        if(evnum==0)  // Get rid of shared tracks
        {
          if((fEvt)->fK0Particle[i].fDaughterID1 == (fEvt+evnum)->fK0Particle[j].fDaughterID1) continue;
	  	  if((fEvt)->fK0Particle[i].fDaughterID1 == (fEvt+evnum)->fK0Particle[j].fDaughterID2) continue;
	      if((fEvt)->fK0Particle[i].fDaughterID2 == (fEvt+evnum)->fK0Particle[j].fDaughterID1) continue;
	      if((fEvt)->fK0Particle[i].fDaughterID2 == (fEvt+evnum)->fK0Particle[j].fDaughterID2) continue;
	    }
	     
        px1 = (fEvt)->fK0Particle[i].fMomentum[0];
		py1 = (fEvt)->fK0Particle[i].fMomentum[1];
		pz1 = (fEvt)->fK0Particle[i].fMomentum[2];
        //pt1 = (fEvt)->fK0Particle[i].fPt;
		px2 = (fEvt+evnum)->fK0Particle[j].fMomentum[0];
		py2 = (fEvt+evnum)->fK0Particle[j].fMomentum[1];
		pz2 = (fEvt+evnum)->fK0Particle[j].fMomentum[2];
        //pt2 = (fEvt+evnum)->fK0Particle[j].fPt;
        if(fRandomNumber->Rndm() < .5){	//switch particle order for 3D qout bias
		 double tempvar;
         tempvar = px1; px1 = px2; px2 = tempvar;
         tempvar = py1; py1 = py2; py2 = tempvar;
         tempvar = pz1; pz1 = pz2; pz2 = tempvar;
		}

		en1  = sqrt(pow(px1,2)+pow(py1,2)+pow(pz1,2)+pow(kMassK0Short,2));
		en2  = sqrt(pow(px2,2)+pow(py2,2)+pow(pz2,2)+pow(kMassK0Short,2));

        q0 = en1 - en2;
        qx = px1 - px2;
        qy = py1 - py2;
        qz = pz1 - pz2;
        qinv = sqrt(pow(qx,2) + pow(qy,2) + pow(qz,2) - pow(q0,2));

        pairPx = px1 + px2;
		pairPy = py1 + py2;
		pairPz = pz1 + pz2;
        pairP0 = en1 + en2;
		pairPt = sqrt(pairPx*pairPx + pairPy*pairPy);
        pairKt = pairPt/2.;									//used for KT binning
        pairMt = sqrt(pairP0*pairP0 - pairPz*pairPz);		//used for LCMS (not plots)
		pairMInv = sqrt(pow(pairP0,2)-pow(pairPx,2)-pow(pairPy,2)-pow(pairPz,2));//used for PRF
        pairPDotQ = pairP0*q0-pairPx*qx-pairPy*qy-pairPz*qz;	//used for PRF

	    //PRF (this section will probably be removed in favor of later boosting section)
        p12 = sqrt(pow(pairPx,2)+pow(pairPy,2)+pow(pairPz,2));	//pair momentum length
        am12 = sqrt(pow(en1+en2,2)-p12*p12);					//sqrt(s)=|p1+p2|(4vec)
        epm = en1+en2+am12;										//"energy plus mass"
        p112 = px1*pairPx+py1*pairPy+pz1*pairPz;				//proj. of p1 on pairP
        if(am12 == 0) continue;
        h1 = (p112/epm - en1)/am12;
        ppx = px1+pairPx*h1;									//px in PRF
        ppy = py1+pairPy*h1;									//py in PRF	
        ppz = pz1+pairPz*h1;									//pz in PRF
        ks = sqrt(ppx*ppx+ppy*ppy+ppz*ppz);						//k*
        ((TH1F*)fOutputList->FindObject("fHistPxCM"))->Fill(ppx);
        ((TH1F*)fOutputList->FindObject("fHistPyCM"))->Fill(ppy);
        ((TH1F*)fOutputList->FindObject("fHistPzCM"))->Fill(ppz);
        ((TH1F*)fOutputList->FindObject("fHistKsCM"))->Fill(ks);
        
        //relative momentum in out-side-long for LCMS and PRF
        if(pairMt == 0 || pairPt == 0) continue;
        qLong = (pairP0*qz - pairPz*q0)/pairMt;	//same for both frames
        qSide = (pairPx*qy - pairPy*qx)/pairPt;	//same for both frames
        //qOutLCMS = (pairPx*qx + pairPy*qy)/pairPt;
	 	qOutPRF  = pairMInv*(pairPx*qx+pairPy*qy)/pairMt/pairPt - pairPt*pairPDotQ/pairMt/pairMInv;
        
		//finding gamma for gamma binning/hists (likely will be removed after tests)
		p1LCMSOut  = (pairPx*px1+pairPy*py1)/pairPt;
		p1LCMSSide = (pairPx*py1-pairPy*px1)/pairPt;
		p1LCMSLong = (pairP0*pz1-pairPz*en1)/pairMt;
		p2LCMSOut  = (pairPx*px2+pairPy*py2)/pairPt;
		p2LCMSSide = (pairPx*py2-pairPy*px2)/pairPt;
		p2LCMSLong = (pairP0*pz2-pairPz*en2)/pairMt;
		en1LCMS	= sqrt(pow(p1LCMSOut,2)+pow(p1LCMSSide,2)+pow(p1LCMSLong,2)+pow(kMassK0Short,2));
		en2LCMS	= sqrt(pow(p2LCMSOut,2)+pow(p2LCMSSide,2)+pow(p2LCMSLong,2)+pow(kMassK0Short,2));		
		betasq = pow((p1LCMSOut+p2LCMSOut)/(en1LCMS+en2LCMS),2);
		gamma = 1./sqrt(1-betasq);
		((TH2F*)fOutputList->FindObject("fHistGamma"))->Fill(gamma,qinv);
		((TH1F*)fOutputList->FindObject("fHistPOutLCMS"))->Fill(p1LCMSOut);
		((TH1F*)fOutputList->FindObject("fHistPSideLCMS"))->Fill(p1LCMSSide);
		((TH1F*)fOutputList->FindObject("fHistPLongLCMS"))->Fill(p1LCMSLong);
		((TH1F*)fOutputList->FindObject("fHistPOutLCMS"))->Fill(p2LCMSOut);
		((TH1F*)fOutputList->FindObject("fHistPSideLCMS"))->Fill(p2LCMSSide);
		((TH1F*)fOutputList->FindObject("fHistPLongLCMS"))->Fill(p2LCMSLong);
		//getting bin numbers and names for 3D histogram
        TString *histname3D = new TString("fHist3DOSL");
        int ktBin;
        if(pairKt < 0.6) ktBin = 0;
		else if(pairKt < 0.8) ktBin = 1;
		else if(pairKt < 1.0) ktBin = 2;
		else ktBin = 3;
		*histname3D += centBin-6; //centBins: [6,15] -> array bins: [0,9]
		*histname3D += ktBin;

        //Spherical harmonics
        //qLength = sqrt(qLong*qLong + qSide*qSide + qOutPRF*qOutPRF);
        //thetaSHCos = qLong/qLength;
        //thetaSH = acos(thetaSHCos);
        //phiSH = acos(qOutPRF/(qLength*sin(thetaSH)));

        //Finding average separation of daughters throughout TPC - two-track cut
        float posPositions1[9][3] = {{0}};
        float negPositions1[9][3] = {{0}};
        float posPositions2[9][3] = {{0}};
        float negPositions2[9][3] = {{0}};
        for(int iPos = 0; iPos < 9; iPos++){
         for(int jPos = 0; jPos < 3; jPos++){
           posPositions1[iPos][jPos] = (fEvt)->fK0Particle[i].fPosXYZ[iPos][jPos];
           negPositions1[iPos][jPos] = (fEvt)->fK0Particle[i].fNegXYZ[iPos][jPos];
           posPositions2[iPos][jPos] = (fEvt+evnum)->fK0Particle[j].fPosXYZ[iPos][jPos];
           negPositions2[iPos][jPos] = (fEvt+evnum)->fK0Particle[j].fNegXYZ[iPos][jPos];
         }
        }    
        float pMean = 0.;	//average separation for positive daughters
        float nMean = 0.;	//average separation for negative daughters
        float pDiff;		
        float nDiff;
        float pMin = 9999.;	//minimum separation (updates) - pos
        float nMin = 9999.;	//minimum separation (updates) - neg
        double pCount=0;  	//counter for number of points used - pos
        double nCount=0;	//counter for number of points used - neg
        for(int ss=0;ss<9;ss++){
         if(posPositions1[ss][0] != -9999 && posPositions2[ss][0] != -9999){          
          pCount++;
          pDiff = sqrt(pow(posPositions1[ss][0]-posPositions2[ss][0],2)+pow(posPositions1[ss][1]-posPositions2[ss][1],2)+pow(posPositions1[ss][2]-posPositions2[ss][2],2));
          pMean = pMean + pDiff;
          if(pDiff < pMin) pMin = pDiff;
         }
         if(negPositions1[ss][0] != -9999 && negPositions1[ss][0] != -9999){
          nCount++;
          nDiff = sqrt(pow(negPositions1[ss][0]-negPositions2[ss][0],2)+pow(negPositions1[ss][1]-negPositions2[ss][1],2)+pow(negPositions1[ss][2]-negPositions2[ss][2],2));     
          nMean = nMean + nDiff;
          if(nDiff < nMin) nMin = nDiff;
         }
        }
        pMean = pMean/pCount; 
        nMean = nMean/nCount;     

        if(evnum==0){ 
         ((TH1F*)fOutputList->FindObject("fHistSepNumPos"))->Fill(pMean); 
         ((TH1F*)fOutputList->FindObject("fHistSepNumNeg"))->Fill(nMean);
         ((TH2F*)fOutputList->FindObject("fHistSepNumPos2"))->Fill(pMean,pMin);
         ((TH2F*)fOutputList->FindObject("fHistSepNumNeg2"))->Fill(nMean,nMin);
        }
        else{
         ((TH1F*)fOutputList->FindObject("fHistSepDenPos"))->Fill(pMean); 
         ((TH1F*)fOutputList->FindObject("fHistSepDenNeg"))->Fill(nMean);
         ((TH2F*)fOutputList->FindObject("fHistSepDenPos2"))->Fill(pMean,pMin);
         ((TH2F*)fOutputList->FindObject("fHistSepDenNeg2"))->Fill(nMean,nMin);
        }

        //Decay plane coincidence
        //daughter momenta
        float a1 = (fEvt)->fK0Particle[i].fPPos[0];
        float b1 = (fEvt)->fK0Particle[i].fPPos[1];
        float c1 = (fEvt)->fK0Particle[i].fPPos[2];
        float d1 = (fEvt)->fK0Particle[i].fPNeg[0];
        float e1 = (fEvt)->fK0Particle[i].fPNeg[1];
        float f1 = (fEvt)->fK0Particle[i].fPNeg[2];
        float a2 = (fEvt+evnum)->fK0Particle[j].fPPos[0];
        float b2 = (fEvt+evnum)->fK0Particle[j].fPPos[1];
        float c2 = (fEvt+evnum)->fK0Particle[j].fPPos[2];
        float d2 = (fEvt+evnum)->fK0Particle[j].fPNeg[0];
        float e2 = (fEvt+evnum)->fK0Particle[j].fPNeg[1];
        float f2 = (fEvt+evnum)->fK0Particle[j].fPNeg[2];
 
        float cross1[3];
        float cross2[3];
        cross1[0] = b1*f1-c1*e1;
        cross1[1] = c1*d1-a1*f1;
        cross1[2] = a1*e1-b1*d1;
        cross2[0] = b2*f2-c2*e2;
        cross2[1] = c2*d2-a2*f2;
        cross2[2] = a2*e2-b2*d2;
        float crosslength1 = sqrt(pow(cross1[0],2)+pow(cross1[1],2)+pow(cross1[2],2));
        float crosslength2 = sqrt(pow(cross2[0],2)+pow(cross2[1],2)+pow(cross2[2],2));
        float dpc = (cross1[0]*cross2[0]+cross1[1]*cross2[1]+cross1[2]*cross2[2])/(crosslength1*crosslength2);

        if(evnum==0)((TH2F*)fOutputList->FindObject("fHistSepDPC"))->Fill(dpc,pMean);
        else ((TH2F*)fOutputList->FindObject("fHistSepDPCBkg"))->Fill(dpc,pMean);
       
        if(pMean < kMinSeparation || nMean < kMinSeparation) continue; //using the "new" method (ala Hans)
        //end separation studies

        //Fill Histograms
        bool center1K0   = kFALSE;  //accepted mass K0
	    bool center2K0   = kFALSE;
        if((fEvt)->fK0Particle[i].fK0) center1K0=kTRUE;
        if((fEvt+evnum)->fK0Particle[j].fK0) center2K0=kTRUE;
        //bool CL1 = kFALSE;
        //bool CL2 = kFALSE;
        //bool CR1 = kFALSE;
        //bool CR2 = kFALSE;
        //if(center1K0 && center2K0){
        // if((fEvt)->fK0Particle[i].fMass < kMassK0Short) CL1 = kTRUE;
        // else CR1 = kTRUE;
        // if((fEvt+evnum)->fK0Particle[j].fMass < kMassK0Short) CL2 = kTRUE;
        // else CR2 = kTRUE;
        //}
       
        //bool SideLeft1 = kFALSE;
        //bool SideLeft2 = kFALSE;
        //bool SideRight1 = kFALSE;
        //bool SideRight2 = kFALSE;
        //if((fEvt)->fK0Particle[i].fSideLeft) SideLeft1 = kTRUE;
        //else if((fEvt)->fK0Particle[i].fSideRight) SideRight1 = kTRUE;
        //if((fEvt+evnum)->fK0Particle[j].fSideLeft) SideLeft2 = kTRUE;
        //else if((fEvt+evnum)->fK0Particle[j].fSideRight) SideRight2 = kTRUE;
	

        if(evnum==0) //Same Event
        {     
          //((TH3F*)fOutputList->FindObject("fHistMassQKt"))->Fill(qinv, pairKt, (fEvt)->fK0Particle[i].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassQKt"))->Fill(qinv, pairKt, (fEvt+evnum)->fK0Particle[j].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassKtK0"))->Fill(centBin+1, pairKt, (fEvt)->fK0Particle[i].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassKtK0"))->Fill(centBin+1, pairKt, (fEvt+evnum)->fK0Particle[j].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassPtCFK0"))->Fill(centBin+1, pt1, (fEvt)->fK0Particle[i].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassPtCFK0"))->Fill(centBin+1, pt2, (fEvt+evnum)->fK0Particle[j].fMass);

          if(center1K0 && center2K0){
           //1D
           ((TH3F*)fOutputList->FindObject("fHistQinvSignal"))->Fill(centBin+1, pairKt, qinv);
           //if(!splitK0centers)((TH3F*)fOutputList->FindObject("fHistQinvSignalNoSplit"))->Fill(centBin+1, pairKt, qinv);
           ((TH2F*)fOutputList->FindObject("fHistKtK0"))->Fill(centBin+1, pairKt);
           
           //for mass bin study
           //if(CL1 && CL2) ((TH3F*)fOutputList->FindObject("fHistCLCLSignal"))->Fill(centBin+1, pairKt, qinv);	   
           //else if ((CL1 && CR2) || (CR1 && CL2)) ((TH3F*)fOutputList->FindObject("fHistCLCRSignal"))->Fill(centBin+1, pairKt, qinv);
           //else ((TH3F*)fOutputList->FindObject("fHistCRCRSignal"))->Fill(centBin+1, pairKt, qinv);

           //3D
           if(pairKt > 0.2 && pairKt < 1.5 && centBin > 5){
		    histname3D->Append("Signal");
			((TH3F*)fOutputList->FindObject(histname3D->Data()))->Fill(qOutPRF,qSide,qLong);
		   }
             
           /*if(pairKt < 1.0){
            if(centBin > 13){
             ((TH3F*)fOutputList->FindObject("fHistOSLCentLowKt"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHCentLowKt"))->Fill(qLength,thetaSHCos,phiSH);}
            else if(centBin > 9){
             ((TH3F*)fOutputList->FindObject("fHistOSLSemiCentLowKt"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHSemiCentLowKt"))->Fill(qLength,thetaSHCos,phiSH);}}            
           else if(pairKt < 2.0){
            if(centBin > 13){
             ((TH3F*)fOutputList->FindObject("fHistOSLCentHighKt"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHCentHighKt"))->Fill(qLength,thetaSHCos, phiSH);}
            else if(centBin > 9){
             ((TH3F*)fOutputList->FindObject("fHistOSLSemiCentHighKt"))->Fill(qOutPRF,qSide,qLong);

             ((TH3F*)fOutputList->FindObject("fHistSHSemiCentHighKt"))->Fill(qLength, thetaSHCos, phiSH);}}*/			

          }//centercenter

          //side-side correlations
          //if(!splitK0sides){
          // if(SideLeft1 && SideLeft2) ((TH3F*)fOutputList->FindObject("fHistLeftLeftSignal"))->Fill(centBin+1, pairKt, qinv);
           //else if((SideLeft1 && SideRight2) || (SideRight1 && SideLeft2)) ((TH3F*)fOutputList->FindObject("fHistLeftRightSignal"))->Fill(centBin+1, pairKt, qinv);
           //else if(SideRight1 && SideRight2) ((TH3F*)fOutputList->FindObject("fHistRightRightSignal"))->Fill(centBin+1, pairKt, qinv);
         //}

        }//same event

        else //Mixed Events
        {
          //((TH3F*)fOutputList->FindObject("fHistMassKtBkgK0"))->Fill(centBin+1, pairKt, (fEvt)->fK0Particle[i].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassKtBkgK0"))->Fill(centBin+1, pairKt, (fEvt+evnum)->fK0Particle[j].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassPtCFBkgK0"))->Fill(centBin+1, pt1, (fEvt)->fK0Particle[i].fMass);
          //((TH3F*)fOutputList->FindObject("fHistMassPtCFBkgK0"))->Fill(centBin+1, pt2, (fEvt+evnum)->fK0Particle[j].fMass);

          if(center1K0 && center2K0){
           //1D
           ((TH3F*)fOutputList->FindObject("fHistQinvBkg"))->Fill(centBin+1, pairKt, qinv);

           //for mass bin study
           //if(CL1 && CL2) ((TH3F*)fOutputList->FindObject("fHistCLCLBkg"))->Fill(centBin+1, pairKt, qinv);	   
           //else if ((CL1 && CR2) || (CR1 && CL2)) ((TH3F*)fOutputList->FindObject("fHistCLCRBkg"))->Fill(centBin+1, pairKt, qinv);
           //else ((TH3F*)fOutputList->FindObject("fHistCRCRBkg"))->Fill(centBin+1, pairKt, qinv);

           //3D
           if(pairKt > 0.2 && pairKt < 1.5 && centBin > 5){
		    histname3D->Replace(12,6,"Bkg");
			((TH3F*)fOutputList->FindObject(histname3D->Data()))->Fill(qOutPRF,qSide,qLong);
		   }
           /*if(pairKt < 1.0){
            if(centBin > 13){
             ((TH3F*)fOutputList->FindObject("fHistOSLCentLowKtBkg"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHCentLowKtBkg"))->Fill(qLength,thetaSHCos,phiSH);}
            else if(centBin > 9){
             ((TH3F*)fOutputList->FindObject("fHistOSLSemiCentLowKtBkg"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHSemiCentLowKtBkg"))->Fill(qLength,thetaSHCos,phiSH);}}
           else if(pairKt < 2.0){
            if(centBin > 13){
             ((TH3F*)fOutputList->FindObject("fHistOSLCentHighKtBkg"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHCentHighKtBkg"))->Fill(qLength, thetaSHCos, phiSH);}
            else if(centBin > 9){
             ((TH3F*)fOutputList->FindObject("fHistOSLSemiCentHighKtBkg"))->Fill(qOutPRF,qSide,qLong);
             ((TH3F*)fOutputList->FindObject("fHistSHSemiCentHighKtBkg"))->Fill(qLength, thetaSHCos, phiSH);}}*/
          }

          //side-side correlations
          //if(SideLeft1 && SideLeft2) ((TH3F*)fOutputList->FindObject("fHistLeftLeftBkg"))->Fill(centBin+1, pairKt, qinv);
          //else if((SideLeft1 && SideRight2) || (SideRight1 && SideLeft2)) ((TH3F*)fOutputList->FindObject("fHistLeftRightBkg"))->Fill(centBin+1, pairKt, qinv);
          //else if(SideRight1 && SideRight2) ((TH3F*)fOutputList->FindObject("fHistRightRightBkg"))->Fill(centBin+1, pairKt, qinv);

        }//Mixed Events
 
      }//past event
    }//event buffer
  }//current event

  // Post output data.
  PostData(1, fOutputList);
  
  }
//________________________________________________________________________
void AliFemtoK0Analysis::Terminate(Option_t *) 
{
  // Called once at the end of the query
  cout<<"Done"<<endl;

}

//_________________________________________________________________________
void AliFemtoK0Analysis::GetGlobalPositionAtGlobalRadiiThroughTPC(const AliAODTrack *track, const Float_t bfield, Float_t globalPositionsAtRadii[9][3], double PrimaryVertex[3]){
  // Gets the global position of the track at nine different radii in the TPC
  // track is the track you want to propagate
  // bfield is the magnetic field of your event
  // globalPositionsAtRadii is the array of global positions in the radii and xyz
  
  // Initialize the array to something indicating there was no propagation
  for(Int_t i=0;i<9;i++){
    for(Int_t j=0;j<3;j++){
      globalPositionsAtRadii[i][j]=-9999.;
    }
  }

   // Make a copy of the track to not change parameters of the track
  AliExternalTrackParam etp; etp.CopyFromVTrack(track);
  //printf("\nAfter CopyFromVTrack\n");
  //etp.Print();
 
  // The global position of the the track
  Double_t xyz[3]={-9999.,-9999.,-9999.};  

  // Counter for which radius we want
  Int_t iR=0; 
  // The radii at which we get the global positions
  // IROC (OROC) from 84.1 cm to 132.1 cm (134.6 cm to 246.6 cm)
  Float_t Rwanted[9]={85.,105.,125.,145.,165.,185.,205.,225.,245.}; 
  // The global radius we are at
  Float_t globalRadius=0;

  // Propagation is done in local x of the track
  for (Float_t x = etp.GetX();x<247.;x+=1.){ // GetX returns local coordinates
    // Starts at the tracks fX and goes outwards. x = 245 is the outer radial limit
    // of the TPC when the track is straight, i.e. has inifinite pt and doesn't get bent.
    // If the track's momentum is smaller than infinite, it will develop a y-component, which
    // adds to the global radius

    // Stop if the propagation was not succesful. This can happen for low pt tracks
    // that don't reach outer radii
    if(!etp.PropagateTo(x,bfield))break;
    etp.GetXYZ(xyz); // GetXYZ returns global coordinates
    globalRadius = TMath::Sqrt(xyz[0]*xyz[0]+xyz[1]*xyz[1]); //Idea to speed up: compare squared radii

    // Roughly reached the radius we want
    if(globalRadius > Rwanted[iR]){
      
      // Bigger loop has bad precision, we're nearly one centimeter too far, go back in small steps.
      while (globalRadius>Rwanted[iR]){
        x-=.1;
        //      printf("propagating to x %5.2f\n",x);
        if(!etp.PropagateTo(x,bfield))break;
        etp.GetXYZ(xyz); // GetXYZ returns global coordinates
        globalRadius = TMath::Sqrt(xyz[0]*xyz[0]+xyz[1]*xyz[1]); //Idea to speed up: compare squared radii
      }
      //printf("At Radius:%05.2f (local x %5.2f). Setting position to x %4.1f y %4.1f z %4.1f\n",globalRadius,x,xyz[0],xyz[1],xyz[2]);
      globalPositionsAtRadii[iR][0]=xyz[0];
      globalPositionsAtRadii[iR][1]=xyz[1];
      globalPositionsAtRadii[iR][2]=xyz[2];
      //subtract primary vertex, "zero" track for correct mixed-event comparison
      globalPositionsAtRadii[iR][0] -= PrimaryVertex[0];
      globalPositionsAtRadii[iR][1] -= PrimaryVertex[1];
      globalPositionsAtRadii[iR][2] -= PrimaryVertex[2];

      // Indicate we want the next radius    
      iR+=1;
    }
    if(iR>=8){
      // TPC edge reached
      return;
    }
  }
}

bool AliFemtoK0Analysis::CheckMeritCutWinner(int cutChoice, double oldPars[3], double newPars[3]){
 //performs "merit cut" judgement check on v0s with shared daughters, using one of three criteria.
 //if cutChoice = 4, it uses all three criteria, needed 2 of 3 'points'

 bool newV0Wins = kFALSE;
 double pardiff[3] = {newPars[0]-oldPars[0],
                      newPars[1]-oldPars[1],
                      newPars[2]-oldPars[2]};
 if(cutChoice > 0 && cutChoice < 4){
  if(pardiff[cutChoice] <= 0.) newV0Wins = kTRUE;
 }
 else if(cutChoice == 4){
  int newWinCount = 0;
  for(int i=0;i<3;i++){if(pardiff[i+1] <= 0) newWinCount++;}
  if(newWinCount > 1) newV0Wins = kTRUE;  
 }
 else{};
 return newV0Wins;
}


