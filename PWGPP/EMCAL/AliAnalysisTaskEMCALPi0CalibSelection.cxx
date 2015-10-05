/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *

 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

// Root 
#include <TRefArray.h>
#include <TList.h>
#include <TH1F.h>
#include <TGeoManager.h>
#include <TFile.h>

// AliRoot
#include "AliAnalysisTaskEMCALPi0CalibSelection.h"
#include "AliAODEvent.h"
#include "AliESDEvent.h"
#include "AliEMCALGeometry.h"
#include "AliVCluster.h"
#include "AliVCaloCells.h"
#include "AliEMCALRecoUtils.h"
#include "AliOADBContainer.h"

/// \cond CLASSIMP
ClassImp(AliAnalysisTaskEMCALPi0CalibSelection) ;
/// \endcond

///
/// Default constructor. Arrays initialization is done here.
//______________________________________________________________________________________________
AliAnalysisTaskEMCALPi0CalibSelection::AliAnalysisTaskEMCALPi0CalibSelection() :
AliAnalysisTaskSE(),  
fEMCALGeo(0x0),           fLoadMatrices(0),
fEMCALGeoName("EMCAL_COMPLETE12SMV1_DCAL_8SM"),
fTriggerName("EMC"),      
fRecoUtils(new AliEMCALRecoUtils), 
fOADBFilePath(""),        fCalibFilePath(""),
fCorrectClusters(kFALSE), fRecalPosition(kTRUE),
fCaloClustersArr(0x0),    fEMCALCells(0x0),
//fCuts(0x0),               
fOutputContainer(0x0),
fVertex(),                fFilteredInput(kFALSE),
fImportGeometryFromFile(1), fImportGeometryFilePath(""),
fEmin(0.5),               fEmax(15.),      
fL0min(0.01),             fL0max(0.5),              
fDTimeCut(100.),          fTimeMax(1000000),        fTimeMin(-1000000),
fAsyCut(1.),              fMinNCells(2),            fGroupNCells(0),
fLogWeight(4.5),          fSameSM(kFALSE),         
fNMaskCellColumns(11),    fMaskCellColumns(0x0),
fInvMassCutMin(110.),     fInvMassCutMax(160.),
// Histograms binning
fNbins(300),              
fMinBin(0.),              fMaxBin(300.),   
fNTimeBins(1000),         fMinTimeBin(0.),          fMaxTimeBin(1000.),
// Temporal
fMomentum1(),             fMomentum2(),             fMomentum12(),
// Histograms
fHmgg(0x0),               fHmggDifferentSM(0x0), 
fHmggMaskFrame(0x0),      fHmggDifferentSMMaskFrame(0x0), 
fHOpeningAngle(0x0),      fHOpeningAngleDifferentSM(0x0),  
fHAsymmetry(0x0),         fHAsymmetryDifferentSM(0x0),  
fhNEvents(0x0),
fhClusterTime(0x0),       fhClusterPairDiffTime(0x0)
{
  for(Int_t iMod=0; iMod < AliEMCALGeoParams::fgkEMCALModules; iMod++)
  {
    for(Int_t iX=0; iX<24; iX++)
    {
      for(Int_t iZ=0; iZ<48; iZ++)
      {
        fHmpi0[iMod][iZ][iX]   = 0 ;
      }
    } 
  }
  
  fVertex[0]=fVertex[1]=fVertex[2]=-1000;
  
  fHTpi0[0]= 0 ;
  fHTpi0[1]= 0 ;
  fHTpi0[2]= 0 ;
  fHTpi0[3]= 0 ;
  
  fMaskCellColumns = new Int_t[fNMaskCellColumns];
  fMaskCellColumns[0] = 6 ;  fMaskCellColumns[1] = 7 ;  fMaskCellColumns[2] = 8 ; 
  fMaskCellColumns[3] = 35;  fMaskCellColumns[4] = 36;  fMaskCellColumns[5] = 37; 
  fMaskCellColumns[6] = 12+AliEMCALGeoParams::fgkEMCALCols; fMaskCellColumns[7] = 13+AliEMCALGeoParams::fgkEMCALCols;
  fMaskCellColumns[8] = 40+AliEMCALGeoParams::fgkEMCALCols; fMaskCellColumns[9] = 41+AliEMCALGeoParams::fgkEMCALCols; 
  fMaskCellColumns[10]= 42+AliEMCALGeoParams::fgkEMCALCols; 
  
  for(Int_t iSMPair = 0; iSMPair < AliEMCALGeoParams::fgkEMCALModules/2; iSMPair++) 
  {
    fHmggPairSameSectorSM[iSMPair]          = 0;
    fHmggPairSameSectorSMMaskFrame[iSMPair] = 0;
    fhClusterPairDiffTimeSameSector[iSMPair]= 0;
  }
  
  for(Int_t iSMPair = 0; iSMPair < AliEMCALGeoParams::fgkEMCALModules-2; iSMPair++)
  { 
    fHmggPairSameSideSM[iSMPair]            = 0;
    fHmggPairSameSideSMMaskFrame[iSMPair]   = 0;
    fhClusterPairDiffTimeSameSide[iSMPair]  = 0;
  }
  
  for(Int_t iSM = 0; iSM < AliEMCALGeoParams::fgkEMCALModules; iSM++) 
  {
    fHmggSM[iSM]                     = 0;
    fHmggSMMaskFrame[iSM]            = 0;
    fHOpeningAngleSM[iSM]            = 0;
    fHOpeningAnglePairSM[iSM]        = 0;
    fHAsymmetrySM[iSM]               = 0;
    fHAsymmetryPairSM[iSM]           = 0;
    fhTowerDecayPhotonHit[iSM]       = 0;
    fhTowerDecayPhotonEnergy[iSM]    = 0;
    fhTowerDecayPhotonAsymmetry[iSM] = 0;
    fhTowerDecayPhotonHitMaskFrame[iSM]= 0;
    fMatrix[iSM]                     = 0x0;
    fhClusterTimeSM[iSM]             = 0;
    fhClusterPairDiffTimeSameSM[iSM] = 0;
  }
}

///
/// Constructor with name as option. Arrays initialization is done here.
///
/// \param name: Name of task.
///
//______________________________________________________________________________________________
AliAnalysisTaskEMCALPi0CalibSelection::AliAnalysisTaskEMCALPi0CalibSelection(const char* name) :
AliAnalysisTaskSE(name),  
fEMCALGeo(0x0),           fLoadMatrices(0),
fEMCALGeoName("EMCAL_COMPLETE12SMV1_DCAL_8SM"),
fTriggerName("EMC"),      
fRecoUtils(new AliEMCALRecoUtils), 
fOADBFilePath(""),        fCalibFilePath(""),
fCorrectClusters(kFALSE), fRecalPosition(kTRUE),
fCaloClustersArr(0x0),    fEMCALCells(0x0),
//fCuts(0x0),               
fOutputContainer(0x0),
fVertex(),                fFilteredInput(kFALSE),
fImportGeometryFromFile(1), fImportGeometryFilePath(""),
fEmin(0.5),               fEmax(15.),      
fL0min(0.01),             fL0max(0.5),              
fDTimeCut(100.),          fTimeMax(1000000),        fTimeMin(-1000000),
fAsyCut(1.),              fMinNCells(2),            fGroupNCells(0),
fLogWeight(4.5),          fSameSM(kFALSE),         
fNMaskCellColumns(11),    fMaskCellColumns(0x0),
fInvMassCutMin(110.),     fInvMassCutMax(160.),
// Histograms binning
fNbins(300),              
fMinBin(0.),              fMaxBin(300.),   
fNTimeBins(1000),         fMinTimeBin(0.),          fMaxTimeBin(1000.),
// Temporal
fMomentum1(),             fMomentum2(),             fMomentum12(),
// Histograms
fHmgg(0x0),               fHmggDifferentSM(0x0), 
fHmggMaskFrame(0x0),      fHmggDifferentSMMaskFrame(0x0), 
fHOpeningAngle(0x0),      fHOpeningAngleDifferentSM(0x0),  
fHAsymmetry(0x0),         fHAsymmetryDifferentSM(0x0),  
fhNEvents(0x0),
fhClusterTime(0x0),       fhClusterPairDiffTime(0x0)
{
  for(Int_t iMod=0; iMod < AliEMCALGeoParams::fgkEMCALModules; iMod++)
  {
    for(Int_t iX=0; iX<24; iX++)
    {
      for(Int_t iZ=0; iZ<48; iZ++)
      {
        fHmpi0[iMod][iZ][iX]   = 0 ;
      }
    } 
  }
  
  fVertex[0]=fVertex[1]=fVertex[2]=-1000;
  
  fHTpi0[0]= 0 ;
  fHTpi0[1]= 0 ;
  fHTpi0[2]= 0 ;
  fHTpi0[3]= 0 ;
  
  fMaskCellColumns = new Int_t[fNMaskCellColumns];
  fMaskCellColumns[0] = 6 ;  fMaskCellColumns[1] = 7 ;  fMaskCellColumns[2] = 8 ; 
  fMaskCellColumns[3] = 35;  fMaskCellColumns[4] = 36;  fMaskCellColumns[5] = 37; 
  fMaskCellColumns[6] = 12+AliEMCALGeoParams::fgkEMCALCols; fMaskCellColumns[7] = 13+AliEMCALGeoParams::fgkEMCALCols;
  fMaskCellColumns[8] = 40+AliEMCALGeoParams::fgkEMCALCols; fMaskCellColumns[9] = 41+AliEMCALGeoParams::fgkEMCALCols; 
  fMaskCellColumns[10]= 42+AliEMCALGeoParams::fgkEMCALCols; 
  
  for(Int_t iSMPair = 0; iSMPair < AliEMCALGeoParams::fgkEMCALModules/2; iSMPair++) 
  {
    fHmggPairSameSectorSM[iSMPair]          = 0;
    fHmggPairSameSectorSMMaskFrame[iSMPair] = 0;
    fhClusterPairDiffTimeSameSector[iSMPair]= 0;
  }
  
  for(Int_t iSMPair = 0; iSMPair < AliEMCALGeoParams::fgkEMCALModules-2; iSMPair++)
  { 
    fHmggPairSameSideSM[iSMPair]            = 0;
    fHmggPairSameSideSMMaskFrame[iSMPair]   = 0;
    fhClusterPairDiffTimeSameSide[iSMPair]  = 0;
  }
  
  for(Int_t iSM = 0; iSM < AliEMCALGeoParams::fgkEMCALModules; iSM++) 
  {
    fHmggSM[iSM]                     = 0;
    fHmggSMMaskFrame[iSM]            = 0;
    fHOpeningAngleSM[iSM]            = 0;
    fHOpeningAnglePairSM[iSM]        = 0;
    fHAsymmetrySM[iSM]               = 0;
    fHAsymmetryPairSM[iSM]           = 0;
    fhTowerDecayPhotonHit[iSM]       = 0;
    fhTowerDecayPhotonEnergy[iSM]    = 0;
    fhTowerDecayPhotonAsymmetry[iSM] = 0;
    fhTowerDecayPhotonHitMaskFrame[iSM]= 0;
    fMatrix[iSM]                     = 0x0;
    fhClusterTimeSM[iSM]             = 0;
    fhClusterPairDiffTimeSameSM[iSM] = 0;
  }
    
  DefineOutput(1, TList::Class());
//DefineOutput(2, TList::Class());  // will contain cuts or local params
}

///
/// Destructor.
//_____________________________________________________________________________
AliAnalysisTaskEMCALPi0CalibSelection::~AliAnalysisTaskEMCALPi0CalibSelection()
{
  if(fOutputContainer)
  {
    fOutputContainer->Delete() ; 
    delete fOutputContainer ;
  }
  
  if(fEMCALGeo)         delete fEMCALGeo  ;
  if(fRecoUtils)        delete fRecoUtils ;
  if(fNMaskCellColumns) delete [] fMaskCellColumns;
}

///
/// Loop over EMCAL clusters and recalibrate and recalculate
/// energy, time and position.
//____________________________________________________________
void  AliAnalysisTaskEMCALPi0CalibSelection::CorrectClusters()
{
  if(fRecoUtils->GetParticleType()!=AliEMCALRecoUtils::kPhoton)
    AliFatal(Form("Wrong particle type for cluster position recalculation! = %d\n", fRecoUtils->GetParticleType()));
    
  AliDebug(1,Form("It will use fLogWeight %.3f",fLogWeight));
    
  Float_t pos[]={0,0,0};
    
  for(Int_t iClu=0; iClu < fCaloClustersArr->GetEntriesFast(); iClu++)
  {
    AliVCluster *c1 = (AliVCluster *) fCaloClustersArr->At(iClu);
      
    Float_t e1i = c1->E();   // cluster energy before correction
    if      (e1i < fEmin) continue;
    else if (e1i > fEmax) continue;
      
    else if (c1->GetNCells() < fMinNCells)                   continue;
      
    else if (c1->GetM02() < fL0min || c1->GetM02() > fL0max) continue;
      
    if(fRecoUtils->ClusterContainsBadChannel(fEMCALGeo, c1->GetCellsAbsId(), c1->GetNCells())) continue;
      
    if(DebugLevel() > 2)
    {
      AliInfo(Form("Std  : i %d, E %f, dispersion %f, m02 %f, m20 %f\n",c1->GetID(),c1->E(),c1->GetDispersion(),c1->GetM02(),c1->GetM20()));
      c1->GetPosition(pos);
      AliInfo(Form("Std  : i %d, x %f, y %f, z %f\n",c1->GetID(), pos[0], pos[1], pos[2]));
    }
      
    // Correct cluster energy and position if requested, and not corrected previously, by default Off
    if(fRecoUtils->IsRecalibrationOn())
    {
      fRecoUtils->RecalibrateClusterEnergy(fEMCALGeo, c1, fEMCALCells);
      fRecoUtils->RecalculateClusterShowerShapeParameters(fEMCALGeo, fEMCALCells,c1);
      fRecoUtils->RecalculateClusterPID(c1);
    }
      
    AliDebug(2,Form("Energy: after recalibration %f",c1->E()));
      
    // Recalculate cluster position
    if ( fRecalPosition ) fRecoUtils->RecalculateClusterPosition(fEMCALGeo, fEMCALCells,c1);
      
    // Correct Non-Linearity
    c1->SetE(fRecoUtils->CorrectClusterEnergyLinearity(c1));
      
    AliDebug(2,Form("after linearity correction %f",c1->E()));
      
    // In case of MC analysis, to match resolution/calibration in real data
    //c1->SetE(fRecoUtils->SmearClusterEnergy(c1)); // Not needed anymore
      
    AliDebug(2,Form("after smearing %f\n",c1->E()));
      
    if(DebugLevel() > 2)
    {
      AliInfo(Form("Cor  : i %d, E %f, dispersion %f, m02 %f, m20 %f\n",c1->GetID(),c1->E(),c1->GetDispersion(),c1->GetM02(),c1->GetM20()));
      c1->GetPosition(pos);
      AliInfo(Form("Cor  : i %d, x %f, y %f, z %f\n",c1->GetID(), pos[0], pos[1], pos[2]));
    }
  } // cluster loop
}

///
/// Fill the invariant mass analysis per channel with the
/// corrected clusters, and other general histograms.
//__________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::FillHistograms()
{
  Int_t absId1   = -1;
  Int_t iSupMod1 = -1;
  Int_t iphi1    = -1;
  Int_t ieta1    = -1;
  Int_t absId2   = -1;
  Int_t iSupMod2 = -1;
  Int_t iphi2    = -1;
  Int_t ieta2    = -1;
  Bool_t shared  = kFALSE;
  
  Float_t pos[] = {0,0,0};
  
  Int_t bc  = InputEvent()->GetBunchCrossNumber();
  Int_t nSM = (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules();

  for(Int_t iClu=0; iClu<fCaloClustersArr->GetEntriesFast()-1; iClu++) 
  {
    AliVCluster *c1 = (AliVCluster *) fCaloClustersArr->At(iClu);
    
    // Exclude bad channels
    if(fRecoUtils->ClusterContainsBadChannel(fEMCALGeo, c1->GetCellsAbsId(), c1->GetNCells())) continue;	
    
    Float_t e1i = c1->E();   // cluster energy before correction   
    
    if      (e1i < fEmin) continue;
    else if (e1i > fEmax) continue;
    
    else if (!fRecoUtils->IsGoodCluster(c1,fEMCALGeo,fEMCALCells,bc)) continue;
    
    else if (c1->GetNCells() < fMinNCells)                        continue; 
    
    else if (c1->GetM02() < fL0min || c1->GetM02() > fL0max)      continue;
    
    if(DebugLevel() > 2)
    { 
      AliInfo(Form("IMA  : i %d, E %f, dispersion %f, m02 %f, m20 %f",c1->GetID(),e1i,c1->GetDispersion(),c1->GetM02(),c1->GetM20()));
      c1->GetPosition(pos);
      AliInfo(Form("IMA  : i %d, x %f, y %f, z %f",c1->GetID(), pos[0], pos[1], pos[2]));
    }
    
    fRecoUtils->GetMaxEnergyCell(fEMCALGeo, fEMCALCells,c1,absId1,iSupMod1,ieta1,iphi1,shared);
    
    c1->GetMomentum(fMomentum1,fVertex);
    
    // Check if cluster is in fidutial region, not too close to borders
    Bool_t in1 = fRecoUtils->CheckCellFiducialRegion(fEMCALGeo, c1, fEMCALCells);
    
    // Clusters not facing frame structures
    Bool_t mask1 = MaskFrameCluster(iSupMod1, ieta1);
    //if(mask1) printf("Reject eta %d SM %d\n",ieta1, iSupMod1);
    
    Double_t time1 = c1->GetTOF()*1.e9;
    
    if(time1 > fTimeMax || time1 < fTimeMin) continue;
    
    fhClusterTime            ->Fill(c1->E(),time1);
    fhClusterTimeSM[iSupMod1]->Fill(c1->E(),time1);
    
    // Combine cluster with other clusters and get the invariant mass
    for (Int_t jClu=iClu+1; jClu < fCaloClustersArr->GetEntriesFast(); jClu++) 
    {
      AliAODCaloCluster *c2 = (AliAODCaloCluster *) fCaloClustersArr->At(jClu);
      
      Float_t e2i = c2->E();
      if      (e2i < fEmin) continue;
      else if (e2i > fEmax) continue;
      
      else if (!fRecoUtils->IsGoodCluster(c2,fEMCALGeo,fEMCALCells,bc))continue;
      
      else if (c2->GetNCells() < fMinNCells)                       continue; 
      
      else if (c2->GetM02() < fL0min || c2->GetM02() > fL0max)     continue;
      
      fRecoUtils->GetMaxEnergyCell(fEMCALGeo, fEMCALCells,c2,absId2,iSupMod2,ieta2,iphi2,shared);
      
      c2->GetMomentum(fMomentum2,fVertex);
      
      fMomentum12 = fMomentum1+fMomentum2;
      Float_t invmass = fMomentum12.M()*1000; 
      
      //Asimetry cut      
      Float_t asym = TMath::Abs(fMomentum1.E()-fMomentum2.E())/(fMomentum1.E()+fMomentum2.E());
      
      if(asym > fAsyCut) continue;
      
      //Time cut
      Double_t time2 = c2->GetTOF()*1.e9;
      
      if(time2 > fTimeMax || time2 < fTimeMin) continue;
      
      fhClusterPairDiffTime->Fill(fMomentum12.E(),time1-time2);
      if(TMath::Abs(time1-time2) > fDTimeCut) continue;
      
      if(invmass < fMaxBin && invmass > fMinBin )
      {
        //Check if cluster is in fidutial region, not too close to borders
        Bool_t in2 = fRecoUtils->CheckCellFiducialRegion(fEMCALGeo, c2, fEMCALCells);
        
        // Clusters not facing frame structures
        Bool_t mask2 = MaskFrameCluster(iSupMod2, ieta2);         
        //if(mask2) printf("Reject eta %d SM %d\n",ieta2, iSupMod2);
        
        if(in1 && in2)
        {
          fHmgg->Fill(invmass,fMomentum12.Pt()); 
          
          if(iSupMod1==iSupMod2) 
          {
            fHmggSM[iSupMod1]->Fill(invmass,fMomentum12.Pt()); 
            fhClusterPairDiffTimeSameSM[iSupMod1]->Fill(fMomentum12.E(),time1-time2);
          }
          else                   
            fHmggDifferentSM ->Fill(invmass,fMomentum12.Pt());
          
          // Same sector
          Int_t j=0;
          for(Int_t i = 0; i < nSM/2; i++)
          {
            j=2*i;
            if((iSupMod1==j && iSupMod2==j+1) || (iSupMod1==j+1 && iSupMod2==j)) 
            {
              fHmggPairSameSectorSM[i]->Fill(invmass,fMomentum12.Pt());
              fhClusterPairDiffTimeSameSector[i]->Fill(fMomentum12.E(),time1-time2);
            } 
          }
          
          // Same side
          for(Int_t i = 0; i < nSM-2; i++)
          {
            if((iSupMod1==i && iSupMod2==i+2) || (iSupMod1==i+2 && iSupMod2==i)) 
            {
              fHmggPairSameSideSM[i]->Fill(invmass,fMomentum12.Pt()); 
              fhClusterPairDiffTimeSameSide[i]->Fill(fMomentum12.E(),time1-time2);
            }
          }
          
          
          if(!mask1 && !mask2)
          {
            fHmggMaskFrame->Fill(invmass,fMomentum12.Pt()); 
            
            if(iSupMod1==iSupMod2) fHmggSMMaskFrame[iSupMod1]->Fill(invmass,fMomentum12.Pt()); 
            else                   fHmggDifferentSMMaskFrame ->Fill(invmass,fMomentum12.Pt());
            
            // Same sector
            j=0;
            for(Int_t i = 0; i < nSM/2; i++)
            {
              j=2*i;
              if((iSupMod1==j && iSupMod2==j+1) || (iSupMod1==j+1 && iSupMod2==j)) fHmggPairSameSectorSMMaskFrame[i]->Fill(invmass,fMomentum12.Pt()); 
            }
            
            // Same side
            for(Int_t i = 0; i < nSM-2; i++)
            {
              if((iSupMod1==i && iSupMod2==i+2) || (iSupMod1==i+2 && iSupMod2==i)) fHmggPairSameSideSMMaskFrame[i]->Fill(invmass,fMomentum12.Pt()); 
            }
            
          }// Pair not facing frame
          
          if(invmass > fInvMassCutMin && invmass < fInvMassCutMax) //restrict to clusters really close to pi0 peak
          {
            
            // Check time of cells in both clusters, and fill time histogram
            for(Int_t icell = 0; icell < c1->GetNCells(); icell++)
            {
              Int_t absID = c1->GetCellAbsId(icell);   
              fHTpi0[bc%4]->Fill(absID, fEMCALCells->GetCellTime(absID)*1.e9);  
            }
            
            for(Int_t icell = 0; icell < c2->GetNCells(); icell++)
            {
              Int_t absID = c2->GetCellAbsId(icell);   
              fHTpi0[bc%4]->Fill(absID, fEMCALCells->GetCellTime(absID)*1.e9);  
            }
            
            //Opening angle of 2 photons
            Float_t opangle = fMomentum1.Angle(fMomentum2.Vect())*TMath::RadToDeg();
            //printf("*******>>>>>>>> In PEAK pt %f, angle %f \n",fMomentum12.Pt(),opangle);
            
            
            fHOpeningAngle ->Fill(opangle,fMomentum12.Pt()); 
            fHAsymmetry    ->Fill(asym,fMomentum12.Pt()); 
            
            if(iSupMod1==iSupMod2) 
            {
              fHOpeningAngleSM[iSupMod1] ->Fill(opangle,fMomentum12.Pt());
              fHAsymmetrySM[iSupMod1]    ->Fill(asym,fMomentum12.Pt());
            }
            else
            {      
              fHOpeningAngleDifferentSM  ->Fill(opangle,fMomentum12.Pt());
              fHAsymmetryDifferentSM     ->Fill(asym,fMomentum12.Pt());
            }
            
            if((iSupMod1==0 && iSupMod2==2) || (iSupMod1==2 && iSupMod2==0)) 
            {
              fHOpeningAnglePairSM[0] ->Fill(opangle,fMomentum12.Pt()); 
              fHAsymmetryPairSM[0]    ->Fill(asym,fMomentum12.Pt());
              
            } 
            if((iSupMod1==1 && iSupMod2==3) || (iSupMod1==3 && iSupMod2==1)) 
            {
              fHOpeningAnglePairSM[1] ->Fill(opangle,fMomentum12.Pt()); 
              fHAsymmetryPairSM[1]    ->Fill(asym,fMomentum12.Pt());
            }
            
            if((iSupMod1==0 && iSupMod2==1) || (iSupMod1==1 && iSupMod2==0)) 
            {
              fHOpeningAnglePairSM[2] ->Fill(opangle,fMomentum12.Pt()); 
              fHAsymmetryPairSM[2]    ->Fill(asym,fMomentum12.Pt());
            }
            if((iSupMod1==2 && iSupMod2==3) || (iSupMod1==3 && iSupMod2==2)) 
            {
              fHOpeningAnglePairSM[3] ->Fill(opangle,fMomentum12.Pt()); 
              fHAsymmetryPairSM[3]    ->Fill(asym,fMomentum12.Pt());
            }
            
          }// pair in 100 < mass < 160
          
        }//in acceptance cuts
        
        //In case of filling only channels with second cluster in same SM
        if(fSameSM && iSupMod1!=iSupMod2) continue;
        
        if (fGroupNCells == 0)
        {
          fHmpi0[iSupMod1][ieta1][iphi1]->Fill(invmass);
          fHmpi0[iSupMod2][ieta2][iphi2]->Fill(invmass);
          
          if(invmass > fInvMassCutMin && invmass < fInvMassCutMax)//restrict to clusters really close to pi0 peak
          {
            fhTowerDecayPhotonHit      [iSupMod1]->Fill(ieta1,iphi1);
            fhTowerDecayPhotonEnergy   [iSupMod1]->Fill(ieta1,iphi1,fMomentum1.E());
            fhTowerDecayPhotonAsymmetry[iSupMod1]->Fill(ieta1,iphi1,asym);
            
            fhTowerDecayPhotonHit      [iSupMod2]->Fill(ieta2,iphi2);
            fhTowerDecayPhotonEnergy   [iSupMod2]->Fill(ieta2,iphi2,fMomentum2.E());
            fhTowerDecayPhotonAsymmetry[iSupMod2]->Fill(ieta2,iphi2,asym);
            
            if(!mask1)fhTowerDecayPhotonHitMaskFrame[iSupMod1]->Fill(ieta1,iphi1);
            if(!mask2)fhTowerDecayPhotonHitMaskFrame[iSupMod2]->Fill(ieta2,iphi2);
            
          }// pair in mass of pi0
        }	
        else
        {
          //printf("Regroup N %d, eta1 %d, phi1 %d, eta2 %d, phi2 %d \n",fGroupNCells, ieta1, iphi1, ieta2, iphi2);
          for (Int_t i = -fGroupNCells; i < fGroupNCells+1; i++) 
          {
            for (Int_t j = -fGroupNCells; j < fGroupNCells+1; j++) 
            {              
              Int_t absId11 = fEMCALGeo->GetAbsCellIdFromCellIndexes(iSupMod1, iphi1+j, ieta1+i); 
              Int_t absId22 = fEMCALGeo->GetAbsCellIdFromCellIndexes(iSupMod2, iphi2+j, ieta2+i); 
              
              Bool_t ok1 = kFALSE;
              Bool_t ok2 = kFALSE;
              
              for(Int_t icell = 0; icell < c1->GetNCells(); icell++)
              {
                if(c1->GetCellsAbsId()[icell] == absId11) ok1=kTRUE;
              }
              
              for(Int_t icell = 0; icell < c2->GetNCells(); icell++)
              {
                if(c2->GetCellsAbsId()[icell] == absId22) ok2=kTRUE;
              }
              
              if(ok1 && (ieta1+i >= 0) && (iphi1+j >= 0) && (ieta1+i < 48) && (iphi1+j < 24))
              {
                fHmpi0[iSupMod1][ieta1+i][iphi1+j]->Fill(invmass);
              }
                
              if(ok2 && (ieta2+i >= 0) && (iphi2+j >= 0) && (ieta2+i < 48) && (iphi2+j < 24))
              {
                fHmpi0[iSupMod2][ieta2+i][iphi2+j]->Fill(invmass);
              }
            }// j loop
          }//i loop
        }//group cells
        
        AliDebug(1,Form("Mass in (SM%d,%d,%d) and  (SM%d,%d,%d): %.3f GeV  E1_i=%f E1_ii=%f  E2_i=%f E2_ii=%f\n",
                        iSupMod1,iphi1,ieta1,iSupMod2,iphi2,ieta2,fMomentum12.M(),e1i,c1->E(),e2i,c2->E()));
      }
    }
  } // end of loop over EMCAL clusters
}


///
/// Recover energy calibration factors from file to be provided by the user
/// via the string fCalibFilePath and pass it to fRecoUtils.
/// Do it only once
//______________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::InitEnergyCalibrationFactors()
{
  if ( !fRecoUtils->IsRecalibrationOn() || fCalibFilePath == "" ) return ;
  
  TFile * calibFactorsFile = TFile::Open(fCalibFilePath);
  
  if ( !calibFactorsFile ) AliFatal("Cannot recover the calibration factors");
  
  for(Int_t ism = 0; ism < fEMCALGeo->GetNumberOfSuperModules(); ism++)
  {
    TH2F * histo = (TH2F*) calibFactorsFile->Get(Form("EMCALRecalFactors_SM%d",ism));
    
    if ( histo ) 
      fRecoUtils->SetEMCALChannelRecalibrationFactors(ism,histo);
    else  
      AliWarning(Form("Null histogram with calibration factors for SM%d, 1 will be used for the full SM!",ism));
  }
}

///
/// Init geometry and set the geometry matrix, for the first event, skip the rest.
/// Also set once the run dependent calibrations.
//________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::InitGeometryMatrices()
{
  Int_t runnumber = InputEvent()->GetRunNumber() ;
  
  //
  // Load default geo matrices if requested
  if(fImportGeometryFromFile && !gGeoManager)
  {
    if(fImportGeometryFilePath=="") // If not specified, set location depending on run number
    {
      // "$ALICE_ROOT/EVE/alice-data/default_geo.root"
      if     (runnumber <  140000) fImportGeometryFilePath = "$ALICE_PHYSICS/OADB/EMCAL/geometry_2010.root";
      else if(runnumber <  171000) fImportGeometryFilePath = "$ALICE_PHYSICS/OADB/EMCAL/geometry_2011.root";
      else if(runnumber <  198000) fImportGeometryFilePath = "$ALICE_PHYSICS/OADB/EMCAL/geometry_2012.root"; // 2012-2013
      else                         fImportGeometryFilePath = "$ALICE_PHYSICS/OADB/EMCAL/geometry_2015.root"; // >= 2015
    }
    
    AliInfo(Form("Import %s",fImportGeometryFilePath.Data()));
    
    TGeoManager::Import(fImportGeometryFilePath) ; // default need file "geometry.root" in local dir!!!!
  }
  
  //
  if(fLoadMatrices)
  {
    AliInfo("Load user defined EMCAL geometry matrices");
    // OADB if available
    AliOADBContainer emcGeoMat("AliEMCALgeo");
    
    if(fOADBFilePath=="") fOADBFilePath = "$ALICE_PHYSICS/OADB/EMCAL" ;
    
    emcGeoMat.InitFromFile(Form("%s/EMCALlocal2master.root",fOADBFilePath.Data()),"AliEMCALgeo");
    
    TObjArray *matEMCAL=(TObjArray*)emcGeoMat.GetObject(runnumber,"EmcalMatrices");
    
    for(Int_t mod = 0; mod < (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules(); mod++)
    {
      if (!fMatrix[mod]) // Get it from OADB
      {
        AliDebug(1,Form("EMCAL matrices SM %d, %p",mod,((TGeoHMatrix*) matEMCAL->At(mod))));
        //((TGeoHMatrix*) matEMCAL->At(mod))->Print();
        
        fMatrix[mod] = (TGeoHMatrix*) matEMCAL->At(mod) ;
      }        
      
      if(fMatrix[mod])
      {
        if(DebugLevel() > 1) 
          fMatrix[mod]->Print();
        
        fEMCALGeo->SetMisalMatrix(fMatrix[mod],mod) ;  
      }
      else if(gGeoManager)
      {
        AliWarning(Form("Set matrix for SM %d from gGeoManager",mod));
        fEMCALGeo->SetMisalMatrix(fEMCALGeo->GetMatrixForSuperModuleFromGeoManager(mod),mod) ;
      }
      else
      {
        AliError(Form("Alignment matrix for SM %d is not available",mod));
      }
    }//SM loop
  }//Load matrices
  else if(!gGeoManager)
  {
    AliInfo("Get geo matrices from data");
    //Still not implemented in AOD, just a workaround to be able to work at least with ESDs	
    if(!strcmp(InputEvent()->GetName(),"AliAODEvent")) 
    {
      AliWarning("Use ideal geometry, values geometry matrix not kept in AODs");
    }//AOD
    else 
    {	
      AliDebug(1,"AliAnalysisTaskEMCALClusterize Load Misaligned matrices");
      
      for(Int_t mod=0; mod < (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules(); mod++)
      {
        if(InputEvent()->GetEMCALMatrix(mod)) 
        {
          if(DebugLevel() > 1) 
            InputEvent()->GetEMCALMatrix(mod)->Print();
          
          fEMCALGeo->SetMisalMatrix(InputEvent()->GetEMCALMatrix(mod),mod) ;
        }
        
      }
    }// ESD
  }// Load matrices from Data
  else if(gGeoManager) // Load default matrices
  {
    for(Int_t mod = 0; mod < (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules(); mod++)
    {
      AliWarning(Form("Set matrix for SM %d from gGeoManager",mod));
      fEMCALGeo->SetMisalMatrix(fEMCALGeo->GetMatrixForSuperModuleFromGeoManager(mod),mod) ;
    }
  } // gGeoManager matrices

}

///
/// Recover the run dependent calibration correction and pass it to fRecoUtils.
/// Do it only once.
//______________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::InitTemperatureCorrections()
{
  if(!fRecoUtils->IsRunDepRecalibrationOn()) return;
  
  AliOADBContainer *contRFTD=new AliOADBContainer("");
  
  contRFTD->InitFromFile(Form("%s/EMCALTemperatureCorrCalib.root",fOADBFilePath.Data()),"AliEMCALRunDepTempCalibCorrections");
  
  Int_t runnumber = InputEvent()->GetRunNumber() ;

  TH1S *htd=(TH1S*)contRFTD->GetObject(runnumber); 
  
  //If it did not exist for this run, get closes one
  if (!htd)
  {
    AliWarning(Form("No TemperatureCorrCalib Objects for run: %d",runnumber));
      
    // let's get the closest runnumber instead then..
    Int_t lower = 0;
    Int_t ic = 0;
    Int_t maxEntry = contRFTD->GetNumberOfEntries();
    
    while ( (ic < maxEntry) && (contRFTD->UpperLimit(ic) < runnumber) )
    {
      lower = ic;
      ic++;
    }
    
    Int_t closest = lower;
    if ( (ic<maxEntry) &&
        (contRFTD->LowerLimit(ic)-runnumber) < (runnumber - contRFTD->UpperLimit(lower)) )
    {
      closest = ic;
    }
    
    AliWarning(Form("TemperatureCorrCalib Objects found closest id %d from run: %d",
                    closest, contRFTD->LowerLimit(closest)));
      
    htd = (TH1S*) contRFTD->GetObjectByIndex(closest);
  } 

  // Fill parameters
  if(htd)
  {
    AliInfo("Recalibrate (Temperature) EMCAL");

    Int_t nSM = fEMCALGeo->GetNumberOfSuperModules();

    for (Int_t ism = 0; ism < nSM; ++ism) 
    {        
      for (Int_t icol = 0; icol < 48; ++icol) 
      {        
        for (Int_t irow = 0; irow < 24; ++irow) 
        {
          Float_t factor = fRecoUtils->GetEMCALChannelRecalibrationFactor(ism,icol,irow);

          Int_t absID = fEMCALGeo->GetAbsCellIdFromCellIndexes(ism, irow, icol); // original calibration factor
          
          AliDebug(3,Form(" ism %d, icol %d, irow %d,absID %d - Calib factor %1.5f - ",ism, icol, irow, absID, factor));

          factor *= htd->GetBinContent(absID) / 10000. ; // correction dependent on T
          
          fRecoUtils->SetEMCALChannelRecalibrationFactor(ism,icol,irow,factor);
            
          AliDebug(3,Form("T  factor %1.5f - final factor %1.5f",
                          htd->GetBinContent(absID) / 10000.,
                          fRecoUtils->GetEMCALChannelRecalibrationFactor(ism,icol,irow)));
        } // columns
      } // rows 
    } // SM loop
  }
  else AliInfo("Do NOT recalibrate EMCAL with T variations, no params TH1");
  
  delete contRFTD; 
}


///
/// Create output container, init geometry.
//___________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::UserCreateOutputObjects()
{
  fEMCALGeo =  AliEMCALGeometry::GetInstance(fEMCALGeoName) ;	
  Int_t nSM = (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules();
  
  fOutputContainer = new TList();
  const Int_t buffersize = 255;
  char hname[buffersize], htitl[buffersize];
  
  fhNEvents        = new TH1I("hNEvents", "Number of analyzed events"   , 1 , 0 , 1  ) ;
  fOutputContainer->Add(fhNEvents);
  
  fHmgg = new TH2F("hmgg","2-cluster invariant mass",fNbins,fMinBin,fMaxBin,100,0,10);
  fHmgg->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
  fHmgg->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHmgg);
  
  fHmggDifferentSM = new TH2F("hmggDifferentSM","2-cluster invariant mass, different SM",fNbins,fMinBin,fMaxBin,100,0,10);
  fHmggDifferentSM->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
  fHmggDifferentSM->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHmggDifferentSM);
  
  fHOpeningAngle = new TH2F("hopang","2-cluster opening angle",100,0.,50.,100,0,10);
  fHOpeningAngle->SetXTitle("#alpha_{#gamma #gamma}");
  fHOpeningAngle->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHOpeningAngle);
  
  fHOpeningAngleDifferentSM = new TH2F("hopangDifferentSM","2-cluster opening angle, different SM",100,0,50.,100,0,10);
  fHOpeningAngleDifferentSM->SetXTitle("#alpha_{#gamma #gamma}");
  fHOpeningAngleDifferentSM->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHOpeningAngleDifferentSM);
   
  fHAsymmetry = new TH2F("hasym","2-cluster opening angle",100,0.,1.,100,0,10);
  fHAsymmetry->SetXTitle("a");
  fHAsymmetry->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHAsymmetry);
  
  fHAsymmetryDifferentSM = new TH2F("hasymDifferentSM","2-cluster opening angle, different SM",100,0,1.,100,0,10);
  fHAsymmetryDifferentSM->SetXTitle("a");
  fHAsymmetryDifferentSM->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHAsymmetryDifferentSM);
  
  //TString pairname[] = {"A side (0-2)", "C side (1-3)","Row 0 (0-1)", "Row 1 (2-3)"};
  
  fHmggMaskFrame = new TH2F("hmggMaskFrame","2-cluster invariant mass, frame masked",fNbins,fMinBin,fMaxBin,100,0,10);
  fHmggMaskFrame->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
  fHmggMaskFrame->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHmggMaskFrame);
  
  fHmggDifferentSMMaskFrame = new TH2F("hmggDifferentSMMaskFrame","2-cluster invariant mass, different SM, frame masked",
                                       fNbins,fMinBin,fMaxBin,100,0,10);
  fHmggDifferentSMMaskFrame->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
  fHmggDifferentSMMaskFrame->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
  fOutputContainer->Add(fHmggDifferentSMMaskFrame);
  
  for(Int_t iSM = 0; iSM < nSM; iSM++) 
  {
    snprintf(hname, buffersize, "hmgg_SM%d",iSM);
    snprintf(htitl, buffersize, "Two-gamma inv. mass for super mod %d",iSM);
    fHmggSM[iSM] = new TH2F(hname,htitl,fNbins,fMinBin,fMaxBin,100,0,10);
    fHmggSM[iSM]->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
    fHmggSM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
    fOutputContainer->Add(fHmggSM[iSM]);
    
    snprintf(hname, buffersize, "hmgg_SM%d_MaskFrame",iSM);
    snprintf(htitl, buffersize, "Two-gamma inv. mass for super mod %d",iSM);
    fHmggSMMaskFrame[iSM] = new TH2F(hname,htitl,fNbins,fMinBin,fMaxBin,100,0,10);
    fHmggSMMaskFrame[iSM]->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
    fHmggSMMaskFrame[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
    fOutputContainer->Add(fHmggSMMaskFrame[iSM]);
    
    if(iSM < nSM/2)
    {
      snprintf(hname,buffersize, "hmgg_PairSameSectorSM%d",iSM);
      snprintf(htitl,buffersize, "Two-gamma inv. mass for SM pair Sector: %d",iSM);
      fHmggPairSameSectorSM[iSM] = new TH2F(hname,htitl,fNbins,fMinBin,fMaxBin,100,0,10);
      fHmggPairSameSectorSM[iSM]->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
      fHmggPairSameSectorSM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
      fOutputContainer->Add(fHmggPairSameSectorSM[iSM]);
      
      snprintf(hname,buffersize, "hmgg_PairSameSectorSM%d_MaskFrame",iSM);
      snprintf(htitl,buffersize, "Two-gamma inv. mass for SM pair Sector: %d",iSM);
      fHmggPairSameSectorSMMaskFrame[iSM] = new TH2F(hname,htitl,fNbins,fMinBin,fMaxBin,100,0,10);
      fHmggPairSameSectorSMMaskFrame[iSM]->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
      fHmggPairSameSectorSMMaskFrame[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
      fOutputContainer->Add(fHmggPairSameSectorSMMaskFrame[iSM]);
      
      fhClusterPairDiffTimeSameSector[iSM] = new TH2F(Form("hClusterPairDiffTimeSameSector%d",iSM),
                                                      Form("cluster pair time difference vs E, Sector %d",iSM),
                                                      100,0,10, 200,-100,100);
      fhClusterPairDiffTimeSameSector[iSM]->SetXTitle("E_{pair} (GeV)");
      fhClusterPairDiffTimeSameSector[iSM]->SetYTitle("#Delta t (ns)");
      fOutputContainer->Add(fhClusterPairDiffTimeSameSector[iSM]);
    }
    
    if(iSM < nSM-2)
    {
      snprintf(hname,buffersize, "hmgg_PairSameSideSM%d",iSM);
      snprintf(htitl,buffersize, "Two-gamma inv. mass for SM pair Sector: %d",iSM);
      fHmggPairSameSideSM[iSM] = new TH2F(hname,htitl,fNbins,fMinBin,fMaxBin,100,0,10);
      fHmggPairSameSideSM[iSM]->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
      fHmggPairSameSideSM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
      fOutputContainer->Add(fHmggPairSameSideSM[iSM]);
      
      snprintf(hname,buffersize, "hmgg_PairSameSideSM%d_MaskFrame",iSM);
      snprintf(htitl,buffersize, "Two-gamma inv. mass for SM pair Sector: %d",iSM);
      fHmggPairSameSideSMMaskFrame[iSM] = new TH2F(hname,htitl,fNbins,fMinBin,fMaxBin,100,0,10);
      fHmggPairSameSideSMMaskFrame[iSM]->SetXTitle("m_{#gamma #gamma} (MeV/c^{2})");
      fHmggPairSameSideSMMaskFrame[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
      fOutputContainer->Add(fHmggPairSameSideSMMaskFrame[iSM]);   
      
      fhClusterPairDiffTimeSameSide[iSM] = new TH2F(Form("hClusterPairDiffTimeSameSide%d",iSM),
                                                    Form("cluster pair time difference vs E,  Side %d",iSM),
                                                    100,0,10, 200,-100,100);
      fhClusterPairDiffTimeSameSide[iSM]->SetXTitle("E_{pair} (GeV)");
      fhClusterPairDiffTimeSameSide[iSM]->SetYTitle("#Delta t (ns)");
      fOutputContainer->Add(fhClusterPairDiffTimeSameSide[iSM]);
    }
    
    snprintf(hname, buffersize, "hopang_SM%d",iSM);
    snprintf(htitl, buffersize, "Opening angle for super mod %d",iSM);
    fHOpeningAngleSM[iSM] = new TH2F(hname,htitl,100,0.,50.,100,0,10);
    fHOpeningAngleSM[iSM]->SetXTitle("#alpha_{#gamma #gamma} (deg)");
    fHOpeningAngleSM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
    fOutputContainer->Add(fHOpeningAngleSM[iSM]);
    
    snprintf(hname,buffersize, "hopang_PairSM%d",iSM);
    snprintf(htitl,buffersize, "Opening angle for SM pair: %d",iSM);
    fHOpeningAnglePairSM[iSM] = new TH2F(hname,htitl,100,0.,50.,100,0,10);
    fHOpeningAnglePairSM[iSM]->SetXTitle("#alpha_{#gamma #gamma} (deg)");
    fHOpeningAnglePairSM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
    fOutputContainer->Add(fHOpeningAnglePairSM[iSM]);    
    
    snprintf(hname, buffersize, "hasym_SM%d",iSM);
    snprintf(htitl, buffersize, "Asymmetry for super mod %d",iSM);
    fHAsymmetrySM[iSM] = new TH2F(hname,htitl,100,0.,1.,100,0,10);
    fHAsymmetrySM[iSM]->SetXTitle("a");
    fHAsymmetrySM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
    fOutputContainer->Add(fHAsymmetrySM[iSM]);
    
    snprintf(hname,buffersize, "hasym_PairSM%d",iSM);
    snprintf(htitl,buffersize, "Asymmetry for SM pair: %d",iSM);
    fHAsymmetryPairSM[iSM] = new TH2F(hname,htitl,100,0.,1.,100,0,10);
    fHAsymmetryPairSM[iSM]->SetXTitle("a");
    fHAsymmetryPairSM[iSM]->SetYTitle("p_{T #gamma #gamma} (GeV/c)");
    fOutputContainer->Add(fHAsymmetryPairSM[iSM]);    
    
    Int_t colmax = 48;
    Int_t rowmax = 24;
    
    fhTowerDecayPhotonHit[iSM]  = new TH2F (Form("hTowerDecPhotonHit_Mod%d",iSM),
                                            Form("Entries in grid of cells in Module %d",iSM), 
                                            colmax+2,-1.5,colmax+0.5, rowmax+2,-1.5,rowmax+0.5); 
    fhTowerDecayPhotonHit[iSM]->SetYTitle("row (phi direction)");
    fhTowerDecayPhotonHit[iSM]->SetXTitle("column (eta direction)");
    fOutputContainer->Add(fhTowerDecayPhotonHit[iSM]);
    
    fhTowerDecayPhotonEnergy[iSM]  = new TH2F (Form("hTowerDecPhotonEnergy_Mod%d",iSM),
                                               Form("Accumulated energy in grid of cells in Module %d",iSM), 
                                               colmax+2,-1.5,colmax+0.5, rowmax+2,-1.5,rowmax+0.5); 
    fhTowerDecayPhotonEnergy[iSM]->SetYTitle("row (phi direction)");
    fhTowerDecayPhotonEnergy[iSM]->SetXTitle("column (eta direction)");
    fOutputContainer->Add(fhTowerDecayPhotonEnergy[iSM]);
    
    fhTowerDecayPhotonAsymmetry[iSM]  = new TH2F (Form("hTowerDecPhotonAsymmetry_Mod%d",iSM),
                                                  Form("Accumulated asymmetry in grid of cells in Module %d",iSM), 
                                                  colmax+2,-1.5,colmax+0.5, rowmax+2,-1.5,rowmax+0.5); 
    fhTowerDecayPhotonAsymmetry[iSM]->SetYTitle("row (phi direction)");
    fhTowerDecayPhotonAsymmetry[iSM]->SetXTitle("column (eta direction)");
    fOutputContainer->Add(fhTowerDecayPhotonAsymmetry[iSM]);
    
    fhTowerDecayPhotonHitMaskFrame[iSM]  = new TH2F (Form("hTowerDecPhotonHit_Mod%d_MaskFrame",iSM),Form("Entries in grid of cells in Module %d",iSM), 
                                                     colmax+2,-1.5,colmax+0.5, rowmax+2,-1.5,rowmax+0.5); 
    fhTowerDecayPhotonHitMaskFrame[iSM]->SetYTitle("row (phi direction)");
    fhTowerDecayPhotonHitMaskFrame[iSM]->SetXTitle("column (eta direction)");
    fOutputContainer->Add(fhTowerDecayPhotonHitMaskFrame[iSM]);
    
    fhClusterTimeSM[iSM] = new TH2F(Form("hClusterTime_SM%d",iSM),"cluster time vs E",100,0,10, 100,0,1000);
    fhClusterTimeSM[iSM]->SetXTitle("E (GeV)");
    fhClusterTimeSM[iSM]->SetYTitle("t (ns)");
    fOutputContainer->Add(fhClusterTimeSM[iSM]);
    
    fhClusterPairDiffTimeSameSM[iSM] = new TH2F(Form("hClusterPairDiffTimeSameSM%d",iSM),
                                                Form("cluster pair time difference vs E, SM %d",iSM),
                                                100,0,10, 200,-100,100);
    fhClusterPairDiffTimeSameSM[iSM]->SetXTitle("E (GeV)");
    fhClusterPairDiffTimeSameSM[iSM]->SetYTitle("#Delta t (ns)");
    fOutputContainer->Add(fhClusterPairDiffTimeSameSM[iSM]);
  }  
  
  Int_t nchannels = nSM*AliEMCALGeoParams::fgkEMCALRows*AliEMCALGeoParams::fgkEMCALCols;
  for(Int_t ibc = 0; ibc < 4; ibc++)
  {
    fHTpi0[ibc] = new TH2F(Form("hTime_BC%d",ibc),Form("Time of cell clusters under pi0 peak, bunch crossing %d",ibc),
                           nchannels,0,nchannels, fNTimeBins,fMinTimeBin,fMaxTimeBin);
    fOutputContainer->Add(fHTpi0[ibc]);       
    fHTpi0[ibc]->SetYTitle("time (ns)");
    fHTpi0[ibc]->SetXTitle("abs. Id. ");
  }
  
  fhClusterTime = new TH2F("hClusterTime","cluster time vs E",100,0,10, 100,0,1000);
  fhClusterTime->SetXTitle("E (GeV)");
  fhClusterTime->SetYTitle("t (ns)");
  fOutputContainer->Add(fhClusterTime);
  
  fhClusterPairDiffTime = new TH2F("hClusterPairDiffTime","cluster pair time difference vs E",100,0,10, 800,-400,400);
  fhClusterPairDiffTime->SetXTitle("E_{pair} (GeV)");
  fhClusterPairDiffTime->SetYTitle("#Delta t (ns)");
  fOutputContainer->Add(fhClusterPairDiffTime);
  
  for(Int_t iMod=0; iMod < nSM; iMod++)
  {
    for(Int_t iRow=0; iRow < AliEMCALGeoParams::fgkEMCALRows; iRow++) 
    {
      for(Int_t iCol=0; iCol < AliEMCALGeoParams::fgkEMCALCols; iCol++) 
      {
        snprintf(hname,buffersize, "%d_%d_%d",iMod,iCol,iRow);
        snprintf(htitl,buffersize, "Two-gamma inv. mass for super mod %d, cell(col,row)=(%d,%d)",iMod,iCol,iRow);
        fHmpi0[iMod][iCol][iRow] = new TH1F(hname,htitl,fNbins,fMinBin,fMaxBin);
        fHmpi0[iMod][iCol][iRow]->SetXTitle("mass (MeV/c^{2})");
        fOutputContainer->Add(fHmpi0[iMod][iCol][iRow]);
      }
    }
  }
  
  fOutputContainer->SetOwner(kTRUE);
    
  PostData(1,fOutputContainer);
  
//  // cuts container, set in terminate but init and post here
//  
//  fCuts = new TList();
//  
//  fCuts ->SetOwner(kTRUE);
//
//  PostData(2, fCuts);
}

///
/// Check if cell is in one of the regions where we have significant amount of material in front of EMCAL.
/// \return True if this cell is in one problematic region
/// \param iSM: supermodule number of the cell.
/// \param ieta: column index of the cell.
//______________________________________________________________________________________________________
Bool_t AliAnalysisTaskEMCALPi0CalibSelection::MaskFrameCluster(Int_t iSM, Int_t ieta) const
{
  Int_t icol = ieta;
  if(iSM%2) icol+=48; // Impair SM, shift index [0-47] to [48-96]
  
  if (fNMaskCellColumns && fMaskCellColumns) 
  {
    for (Int_t imask = 0; imask < fNMaskCellColumns; imask++) 
    {
      if(icol==fMaskCellColumns[imask]) return kTRUE;
    }
  }
  
  return kFALSE;
}

///
/// Main method, do the analysis per event:
/// * first, select the events;
/// * then, correct the clusters if needed;
/// * finally, fill the histograms per channel after recalibration.
//__________________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::UserExec(Option_t* /* option */)
{
  // Event selection
    
  if(fTriggerName!="")
  {
    AliESDEvent* esdevent = dynamic_cast<AliESDEvent*> (InputEvent());
    AliAODEvent* aodevent = dynamic_cast<AliAODEvent*> (InputEvent());
    
    TString triggerClass = ""; 
    if     (esdevent) triggerClass = esdevent->GetFiredTriggerClasses();
    else if(aodevent) triggerClass = aodevent->GetFiredTriggerClasses();
    
   AliDebug(1,Form("Event %d, FiredClass %s",
             (Int_t)Entry(),(((AliESDEvent*)InputEvent())->GetFiredTriggerClasses()).Data()));
    
    if(!triggerClass.Contains(fTriggerName)) 
    {
       AliDebug(1,"Reject event!");
       return;
    }  
    else 
        AliDebug(1,"Accept event!");
  }
  
  // Get the input event
    
  AliVEvent* event = 0;
  if(fFilteredInput) event = AODEvent();
  else               event = InputEvent();
  
  if(!event) 
  {
    AliWarning("Input event not available!");
    return;
  }
  
  AliDebug(1,Form("<<< %s: Event %d >>>",event->GetName(), (Int_t)Entry()));
  
  // Get the primary vertex
    
  event->GetPrimaryVertex()->GetXYZ(fVertex) ;
  
  AliDebug(1,Form("Vertex: (%.3f,%.3f,%.3f)",fVertex[0],fVertex[1],fVertex[2]));
  
  //Int_t runNum = aod->GetRunNumber();
  //if(DebugLevel() > 1) printf("Run number: %d\n",runNum);
  
  fhNEvents->Fill(0); //Count the events to be analyzed

  // Acccess once the geometry matrix and temperature corrections and calibration coefficients
  if(fhNEvents->GetEntries() == 1) 
  {
    InitGeometryMatrices();

    InitTemperatureCorrections();

    InitEnergyCalibrationFactors();
  }
  
  //Get the list of clusters and cells
  fEMCALCells       = event->GetEMCALCells();

  fCaloClustersArr  = new TRefArray();
  event->GetEMCALClusters(fCaloClustersArr);
  
  AliDebug(1,Form("N CaloClusters: %d - N CaloCells %d",fCaloClustersArr->GetEntriesFast(), fEMCALCells->GetNumberOfCells()));
  
 // Apply non linearity, new calibration, T calibration to the clusters
 if( fCorrectClusters )
     CorrectClusters();
  
  FillHistograms();  

  delete fCaloClustersArr;
  
  PostData(1,fOutputContainer);
}

///
/// Print settings.
//_____________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::PrintInfo()
{
  printf("Cluster cuts: %2.2f < E < %2.2f GeV; number of cells > %d; Assymetry < %1.2f, pair time diff < %2.2f, %2.2f < t < %2.2f ns\n", 
         fEmin,fEmax, fMinNCells, fAsyCut, fDTimeCut,fTimeMin,fTimeMax) ;
  
  printf("Group %d cells\n", fGroupNCells) ;
  
  printf("Cluster maximal cell away from border at least %d cells\n", fRecoUtils->GetNumberOfCellsFromEMCALBorder()) ;
  
  printf("Histograms: bins %d; energy range: %2.2f < E < %2.2f MeV\n",fNbins,fMinBin,fMaxBin) ;
  
  printf("Switchs:\n \t Remove Bad Channels? %d; Use filtered input? %d;  Correct Clusters? %d, and their position? %d \n \t Mass per channel same SM clusters? %d\n",
         fRecoUtils->IsBadChannelsRemovalSwitchedOn(),fFilteredInput,fCorrectClusters, fRecalPosition, fSameSM) ;

  printf("OADB path        : %s\n",fOADBFilePath .Data());
  printf("Calibration path : %s\n",fCalibFilePath.Data());
  
  printf("EMCAL Geometry name: < %s >, Load Matrices %d\n",fEMCALGeoName.Data(), fLoadMatrices) ;

  if(fLoadMatrices) { for(Int_t ism = 0; ism < AliEMCALGeoParams::fgkEMCALModules; ism++) if(fMatrix[ism]) fMatrix[ism]->Print() ; }
}

///
/// Set the total number of columns to be masked in the analysis
/// \param n: number of columns
//_____________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::SetNMaskCellColumns(Int_t n)
{
    if(n > fNMaskCellColumns)
    {
        delete [] fMaskCellColumns ;
        
        fMaskCellColumns = new Int_t[n] ;
    }
    
    fNMaskCellColumns = n ;
}

///
/// Define which column must be masked and its position in the array of columns
/// \param ipos: position in the list of columns.
/// \param icol: column to be masked in EMCal mapping coordinate.
//___________________________________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::SetMaskCellColumn(Int_t ipos, Int_t icol)
{
    if(ipos < fNMaskCellColumns) fMaskCellColumns[ipos] = icol            ;
    else AliWarning("Mask column not set, position larger than allocated set size first") ;
}

///
/// Create cuts/param objects and publish to slot. Comment out for the moment.
//______________________________________________________________
void AliAnalysisTaskEMCALPi0CalibSelection::Terminate(Option_t*)
{
  AliDebug(1,"Not implemented");
//  const Int_t buffersize = 255;
//  char onePar[buffersize] ;
  
//  snprintf(onePar,buffersize, "Custer cuts: %2.2f < E < %2.2f GeV; %2.2f < Lambda0_2 < %2.2f GeV; min number of cells %d; Assymetry cut %1.2f, time1-time2 < %2.2f; %2.2f < T < %2.2f ns; %3.1f < Mass < %3.1f", 
//           fEmin,fEmax, fL0min, fL0max, fMinNCells, fAsyCut, fDTimeCut, fTimeMin, fTimeMax, fInvMassCutMin, fInvMassCutMax) ;
//  fCuts->Add(new TObjString(onePar));
//  snprintf(onePar,buffersize, "Group %d cells;", fGroupNCells) ;
//  fCuts->Add(new TObjString(onePar));
//  snprintf(onePar,buffersize, "Cluster maximal cell away from border at least %d cells;", fRecoUtils->GetNumberOfCellsFromEMCALBorder()) ;
//  fCuts->Add(new TObjString(onePar));
//  snprintf(onePar,buffersize, "Histograms, Mass bins %d; energy range: %2.2f < E < %2.2f GeV;",fNbins,fMinBin,fMaxBin) ;
//  fCuts->Add(new TObjString(onePar));
//  snprintf(onePar,buffersize, "Histograms, Time bins %d; energy range: %2.2f < E < %2.2f GeV;",fNTimeBins,fMinTimeBin,fMaxTimeBin) ;
//  fCuts->Add(new TObjString(onePar));
//  snprintf(onePar,buffersize, "Switchs: Remove Bad Channels? %d; Use filtered input? %d;  Correct Clusters? %d and their position? %d, Mass per channel same SM clusters? %d ",
//           fRecoUtils->IsBadChannelsRemovalSwitchedOn(),fFilteredInput,fCorrectClusters, fRecalPosition, fSameSM) ;
//  fCuts->Add(new TObjString(onePar));
//  snprintf(onePar,buffersize, "EMCAL Geometry name: < %s >, Load Matrices? %d",fEMCALGeoName.Data(),fLoadMatrices) ;
//  fCuts->Add(new TObjString(onePar));
//    
//  // Post Data
//  PostData(2, fCuts);
}

