// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//
/// \brief this task allows for the direct one-to-one comparison of
//         cascades computed with standard DCAFitter methods and the KFparticle
//         package. It is meant for the purposes of larger-scale QA of KF reco.

/// \brief cascadebuilder.cxx and lambdakzerobuilder.cxx tasks need to be added to the workflow. Flag createCascCovMats needs to be enabled!

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/ASoAHelpers.h"
#include "ReconstructionDataFormats/Track.h"
#include "Common/Core/RecoDecay.h"
#include "Common/Core/trackUtilities.h"
#include "PWGLF/DataModel/LFStrangenessTables.h"
#include "PWGLF/DataModel/kfStrangenessStudy.h"
#include "Common/Core/TrackSelection.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/Centrality.h"
#include "Common/DataModel/PIDResponse.h"
#include "Common/DataModel/Multiplicity.h"
#include <cmath>

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;
using std::array;

// allows for candidate-by-candidate comparison using Cascade to []CascData link table
using CascadesCrossLinked = soa::Join<aod::Cascades, aod::CascDataLink, aod::KFCascDataLink>;
using CascDataLabeled = soa::Join<aod::CascDatas, aod::CascCovs, aod::McCascLabels>;
using KFCascDataLabeled = soa::Join<aod::KFCascDatas, aod::KFCascCovs, aod::McKFCascLabels>;

struct kfStrangenessStudy {

  Produces<aod::CascCand> rowCasc;
  Produces<aod::CascCandMC> rowCascMC;

  /// Configurables
  Configurable<bool> isMC{"isMC", false, "filling MC table if running over MC"};

  HistogramRegistry histos{"Histos", {}, OutputObjHandlingPolicy::AnalysisObject};

  /// Filters
  Filter collisionFilter = (aod::evsel::sel8 == true);

  /// Cascade data
  int isDCAfitter = 0, isKF = 0;
  float pt = -1.0f, ptKF = -1.0f;
  float ptV0 = -1.0f, ptKFV0 = -1.0f;
  float massXi = -1.0f, massXiKF = -1.0f;
  float massOmega = -1.0f, massOmegaKF = -1.0f;
  float massLambda = -1.0f, massLambdaKF = -1.0f;
  float cascRad = -1.0f, cascRadKF = -1.0f;
  float vtxX = -1.0f, vtxY = -1.0f, vtxZ = -1.0f, vtxXKF = -1.0f, vtxYKF = -1.0f, vtxZKF = -1.0f;
  float vtxXErr = -1.0f, vtxYErr = -1.0f, vtxZErr = -1.0f, vtxXErrKF = -1.0f, vtxYErrKF = -1.0f, vtxZErrKF = -1.0f;
  float vtxXV0 = -1.0f, vtxYV0 = -1.0f, vtxZV0 = -1.0f, vtxXKFV0 = -1.0f, vtxYKFV0 = -1.0f, vtxZKFV0 = -1.0f;
  float vtxXErrV0 = -1.0f, vtxYErrV0 = -1.0f, vtxZErrV0 = -1.0f, vtxXErrKFV0 = -1.0f, vtxYErrKFV0 = -1.0f, vtxZErrKFV0 = -1.0f;
  float dcaXYCascToPV = -1.0f, dcaXYCascToPVKF = -1.0f;
  float dcaZCascToPV = -1.0f, dcaZCascToPVKF = -1.0f;
  float dcaCascDaughters = -1.0f, dcaCascDaughtersKF = -1.0f;
  float dcaV0Daughters = -1.0f, dcaV0DaughtersKF = -1.0f;
  float dcaPosToPV = -1.0f, dcaPosToPVKF = -1.0f;
  float dcaNegToPV = -1.0f, dcaNegToPVKF = -1.0f;
  float dcaBachToPV = -1.0f, dcaBachToPVKF = -1.0f;
  float cascPointingAngle = -1.0f, cascPointingAngleKF = -1.0f;
  float v0PointingAngle = -1.0f, v0PointingAngleKF = -1.0f;
  float V0Rad = -1.0f, V0RadKF = -1.0f;
  int charge = 0;
  float cascChi2geoKF = -1.f;

  /// Additional cascade MC data
  int isTrueCasc = 0;
  float ptRec = -1.0f, ptRecKF = -1.0f, ptGen = -1.0f;
  float ptRecV0 = -1.0f, ptRecKFV0 = -1.0f, ptGenV0 = -1.0f;
  float vtxXrec = -1.0f, vtxYrec = -1.0f, vtxZrec = -1.0f, vtxXrecKF = -1.0f, vtxYrecKF = -1.0f, vtxZrecKF = -1.0f;
  float vtxXrecErr = -1.0f, vtxYrecErr = -1.0f, vtxZrecErr = -1.0f, vtxXrecErrKF = -1.0f, vtxYrecErrKF = -1.0f, vtxZrecErrKF = -1.0f;
  float vtxXrecV0 = -1.0f, vtxYrecV0 = -1.0f, vtxZrecV0 = -1.0f, vtxXrecKFV0 = -1.0f, vtxYrecKFV0 = -1.0f, vtxZrecKFV0 = -1.0f;
  float vtxXrecErrV0 = -1.0f, vtxYrecErrV0 = -1.0f, vtxZrecErrV0 = -1.0f, vtxXrecErrKFV0 = -1.0f, vtxYrecErrKFV0 = -1.0f, vtxZrecErrKFV0 = -1.0f;
  float vtxXgen = -1.0f, vtxYgen = -1.0f, vtxZgen = -1.0f;
  float vtxXgenV0 = -1.0f, vtxYgenV0 = -1.0f, vtxZgenV0 = -1.0f;
  float vtxXgen_firstDau = -1., vtxYgen_firstDau = -1., vtxZgen_firstDau = -1;
  float vtxXgen_lastDau = -1., vtxYgen_lastDau = -1., vtxZgen_lastDau = -1;
  float vtxXgen_firstV0Dau = -1., vtxYgen_firstV0Dau = -1., vtxZgen_firstV0Dau = -1;
  float prodVtxXgen  = -1.0f, prodVtxYgen  = -1.0f, prodVtxZgen  = -1.0f;
  float prodVtxXgenV0  = -1.0f, prodVtxYgenV0  = -1.0f, prodVtxZgenV0  = -1.0f;
  int source = 0;

  void init(InitContext const&)
  {
    /// QA histos
    histos.add("hChargeCounter", "hChargeCounter", kTH1F, {{3, -1.5f, 1.5f}});
    histos.add("hChargeCounterCascDatas", "hChargeCounterCascDatas", kTH1F, {{3, -1.5f, 1.5f}});
    auto hEventSelectionFlow = histos.add<TH1>("hEventSelectionFlow", "Event selection flow", kTH1F, {{2, 0.5f, 2.5f}});
    hEventSelectionFlow->GetXaxis()->SetBinLabel(hEventSelectionFlow->FindBin(1), "Sel8");
    hEventSelectionFlow->GetXaxis()->SetBinLabel(hEventSelectionFlow->FindBin(2), "|Vtx_{z}|<10cm");

    histos.add("hVertexX", "hVertexX", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hKFVertexX", "hKFVertexX", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hVertexY", "hVertexY", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hKFVertexY", "hKFVertexY", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hVertexZ", "hVertexZ", kTH1F, {{1000, -10.0f, 10.0f}});
    histos.add("hKFVertexZ", "hKFVertexZ", kTH1F, {{1000, -10.0f, 10.0f}});

    histos.add("hCascRadius", "hCascRadius", kTH1F, {{1000, 0.0f, 3.0f}});
    histos.add("hKFCascRadius", "hKFCascRadius", kTH1F, {{1000, 0.0f, 3.0f}});

    histos.add("hDCAxy", "hDCAxy", kTH1F, {{500, -1.0f, 1.0f}});
    histos.add("hKFDCAxy", "hKFDCAxy", kTH1F, {{500, -1.0f, 1.0f}});
    
    histos.add("hPointingAngle", "hPointingAngle", kTH1F, {{800, 0.0f, 3.5f}});
    histos.add("hKFPointingAngle", "hKFPointingAngle", kTH1F, {{800, 0.0f, 3.5f}});
    histos.add("hCosPointingAngle", "hCosPointingAngle", kTH1F, {{800, -1.0f, 1.0f}});
    histos.add("hKFCosPointingAngle", "hKFCosPointingAngle", kTH1F, {{800, -1.0f, 1.0f}});
    histos.add("hV0PointingAngle", "hV0PointingAngle", kTH1F, {{800, 0.0f, 3.5f}});
    histos.add("hKFV0PointingAngle", "hKFV0PointingAngle", kTH1F, {{800, 0.0f, 3.5f}});
    histos.add("hCosV0PointingAngle", "hCosV0PointingAngle", kTH1F, {{800, -1.0f, 1.0f}});
    histos.add("hKFCosV0PointingAngle", "hKFCosV0PointingAngle", kTH1F, {{800, -1.0f, 1.0f}});

    histos.add("hGenDecayVtxX_firstDau", "hGenDecayVtxX_firstDau", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hGenDecayVtxY_firstDau", "hGenDecayVtxY_firstDau", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hGenDecayVtxZ_firstDau", "hGenDecayVtxZ_firstDau", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hGenDecayVtxX_lastDau", "hGenDecayVtxX_lastDau", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hGenDecayVtxY_lastDau", "hGenDecayVtxY_lastDau", kTH1F, {{1000, -3.0f, 3.0f}});
    histos.add("hGenDecayVtxZ_lastDau", "hGenDecayVtxZ_lastDau", kTH1F, {{1000, -3.0f, 3.0f}});

    histos.add("hGenSource", "hGenSource", kTH1F, {{5, -2, 3}});
    
  }

  template <typename TCollision, typename TCascade, typename TCascDatas, typename TKFCascDatas>
  void getCascDatas(TCollision const& collision, TCascade const& cascade, TCascDatas const& cascdatas, TKFCascDatas const& kfcascdatas)
  {
    if (cascade.has_cascData()) {
      LOG(info) << "Cascade has CascData!";
      // check aod::Cascades -> aod::CascData link
      // if present: this candidate was accepted by default DCAfitter building
      isDCAfitter = 1;
      auto cascdata = cascade.template cascData_as<TCascDatas>();
      if (isMC) {
        ptRec = cascdata.pt();
        vtxXrec = cascdata.x();
        vtxYrec = cascdata.y();
        vtxZrec = cascdata.z();
        vtxXrecErr = sqrt(cascdata.positionCovMat()[0]);
        vtxYrecErr = sqrt(cascdata.positionCovMat()[2]);
        vtxZrecErr = sqrt(cascdata.positionCovMat()[5]);
        ptRecV0 = sqrt((cascdata.pxpos()+cascdata.pxneg())*(cascdata.pxpos()+cascdata.pxneg()) + (cascdata.pypos()+cascdata.pyneg())*(cascdata.pypos()+cascdata.pyneg())); // taken from daughters from lambdakzerobuiler
        vtxXrecV0 = cascdata.xlambda();
        vtxYrecV0 = cascdata.ylambda();
        vtxZrecV0 = cascdata.zlambda();
        // vtxXrecErrV0 = 
        // vtxYrecErrV0 = 
        // vtxZrecErrV0 = 
      } else {
        pt = cascdata.pt();
        vtxX = cascdata.x();
        vtxY = cascdata.y();
        vtxZ = cascdata.z();
        vtxXErr = sqrt(cascdata.positionCovMat()[0]);
        vtxYErr = sqrt(cascdata.positionCovMat()[2]);
        vtxZErr = sqrt(cascdata.positionCovMat()[5]);
        ptV0 = sqrt((cascdata.pxpos()+cascdata.pxneg())*(cascdata.pxpos()+cascdata.pxneg()) + (cascdata.pypos()+cascdata.pyneg())*(cascdata.pypos()+cascdata.pyneg())); // taken from daughters from lambdakzerobuiler
        vtxXV0 = cascdata.xlambda();
        vtxYV0 = cascdata.ylambda();
        vtxZV0 = cascdata.zlambda();
        // vtxXErrV0 = 
        // vtxYErrV0 = 
        // vtxZErrV0 = 
      }
      massLambda = cascdata.mLambda();
      massXi = cascdata.mXi();
      massOmega = cascdata.mOmega();
      dcaXYCascToPV = cascdata.dcaXYCascToPV();
      dcaZCascToPV = cascdata.dcaZCascToPV();
      dcaCascDaughters = cascdata.dcacascdaughters();
      dcaV0Daughters = cascdata.dcaV0daughters();
      dcaPosToPV = cascdata.dcapostopv();
      dcaNegToPV = cascdata.dcanegtopv();
      dcaBachToPV = cascdata.dcabachtopv();
      cascPointingAngle = TMath::ACos(cascdata.casccosPA(collision.posX(), collision.posY(), collision.posZ()));
      cascRad = cascdata.cascradius();
      V0Rad = cascdata.v0radius();
      v0PointingAngle = TMath::ACos(cascdata.v0cosPA(collision.posX(), collision.posY(), collision.posZ()));

      // fill QA histos
      histos.fill(HIST("hVertexX"), vtxX);  
      histos.fill(HIST("hVertexY"), vtxY);
      histos.fill(HIST("hVertexZ"), vtxZ);
      histos.fill(HIST("hDCAxy"), dcaXYCascToPV);
      histos.fill(HIST("hCascRadius"), cascRad);
      histos.fill(HIST("hPointingAngle"), cascPointingAngle);
      histos.fill(HIST("hCosPointingAngle"), cos(cascPointingAngle));
      histos.fill(HIST("hV0PointingAngle"), v0PointingAngle);
      histos.fill(HIST("hCosV0PointingAngle"), cos(v0PointingAngle));
    }

    if (cascade.has_kfCascData()) {
      LOG(info) << "Cascade has KFcascData!";
      // check aod::Cascades -> aod::KFCascData link
      // if present: this candidate was accepted by KF building
      isKF = 1;
      auto cascdatakf = cascade.template kfCascData_as<TKFCascDatas>();
      if (isMC) {
        ptRecKF = cascdatakf.pt();
        vtxXrecKF = cascdatakf.x();
        vtxYrecKF = cascdatakf.y();
        vtxZrecKF = cascdatakf.z();
        vtxXrecErrKF = sqrt(cascdatakf.kfTrackCovMat()[0]);
        vtxYrecErrKF = sqrt(cascdatakf.kfTrackCovMat()[2]);
        vtxZrecErrKF = sqrt(cascdatakf.kfTrackCovMat()[5]);
        ptRecKFV0 = sqrt(cascdatakf.pxv0()*cascdatakf.pxv0() + cascdatakf.pyv0()*cascdatakf.pyv0()); // taken from KFV0
        vtxXrecKFV0 = cascdatakf.xlambda();
        vtxYrecKFV0 = cascdatakf.ylambda();
        vtxZrecKFV0 = cascdatakf.zlambda();
        // vtxXrecErrKFV0 = 
        // vtxYrecErrKFV0 = 
        // vtxZrecErrKFV0 = 
      } else {
        ptKF = cascdatakf.pt();
        vtxXKF = cascdatakf.x();
        vtxYKF = cascdatakf.y();
        vtxZKF = cascdatakf.z();
        vtxXErrKF = sqrt(cascdatakf.kfTrackCovMat()[0]);
        vtxYErrKF = sqrt(cascdatakf.kfTrackCovMat()[2]);
        vtxZErrKF = sqrt(cascdatakf.kfTrackCovMat()[5]);
        ptKFV0 = sqrt(cascdatakf.pxv0()*cascdatakf.pxv0() + cascdatakf.pyv0()*cascdatakf.pyv0()); // taken from KFV0
        vtxXKFV0 = cascdatakf.xlambda();
        vtxYKFV0 = cascdatakf.ylambda();
        vtxZKFV0 = cascdatakf.zlambda();
        // vtxXErrKFV0 = 
        // vtxYErrKFV0 = 
        // vtxZErrKFV0 = 
      }
      massLambdaKF = cascdatakf.mLambda();
      massXiKF = cascdatakf.mXi();
      massOmegaKF = cascdatakf.mOmega();
      dcaXYCascToPVKF = cascdatakf.dcaXYCascToPV();
      dcaZCascToPVKF = cascdatakf.dcaZCascToPV();
      dcaCascDaughtersKF = cascdatakf.dcacascdaughters();
      dcaV0DaughtersKF = cascdatakf.dcaV0daughters();
      dcaPosToPVKF = cascdatakf.dcapostopv();
      dcaNegToPVKF = cascdatakf.dcanegtopv();
      dcaBachToPVKF = cascdatakf.dcabachtopv();
      cascPointingAngleKF = TMath::ACos(cascdatakf.casccosPA(collision.posX(), collision.posY(), collision.posZ()));
      cascRadKF = cascdatakf.cascradius();
      V0RadKF = cascdatakf.v0radius();
      cascChi2geoKF = cascdatakf.kfCascadeChi2();
      v0PointingAngleKF = TMath::ACos(cascdatakf.v0cosPA(collision.posX(), collision.posY(), collision.posZ()));

      // fill QA histos
      histos.fill(HIST("hKFVertexX"), vtxXKF);
      histos.fill(HIST("hKFVertexY"), vtxYKF);
      histos.fill(HIST("hKFVertexZ"), vtxZKF);
      histos.fill(HIST("hKFDCAxy"), dcaXYCascToPVKF);
      histos.fill(HIST("hKFCascRadius"), cascRadKF);
      histos.fill(HIST("hKFPointingAngle"), cascPointingAngleKF);
      histos.fill(HIST("hKFCosPointingAngle"), cos(cascPointingAngleKF));
      histos.fill(HIST("hKFV0PointingAngle"), v0PointingAngleKF);
      histos.fill(HIST("hKFCosV0PointingAngle"), cos(v0PointingAngleKF));
    }
  }

  template <typename TCollision, typename TCascData, typename TMCParticle>
  void getCascMCdata(TCollision const& collision, TCascData const& cascadata, TMCParticle const& mcparticles) {
    if (cascdata.has_mcParticle() && cascdata.mcParticleId() > -1 && cascdata.mcParticleId() <= mcparticles.size()) {
      auto MCcascade = cascdata.template mcParticle_as<TMCParticle>();
      auto v0 = cascdata.v0Id();
      auto MCv0 = v0.template mcParticle_as<TMCParticle>();
      if (!(abs(MCv0.pdgCode()) == 3122)) {
        LOG(info) << "V0 is no Lambda";
      }

      if (MCcascade.has_daughters()) {
        LOG(info) << "MC cascade has daughters, getting MC info.";
        // cascade
        ptGen = MCcascade.pt();
        prodVtxXgen = MCcascade.vx();
        prodVtxYgen = MCcascade.vy();
        prodVtxZgen = MCcascade.vz();
        vtxXgen_firstDau = MCcascade.template daughters_as<TMCParticle>().begin().vx();
        vtxYgen_firstDau = MCcascade.template daughters_as<TMCParticle>().begin().vy();
        vtxZgen_firstDau = MCcascade.template daughters_as<TMCParticle>().begin().vz();
        vtxXgen_lastDau = (MCcascade.template daughters_as<TMCParticle>().begin() + 1).vx();
        vtxYgen_lastDau = (MCcascade.template daughters_as<TMCParticle>().begin() + 1).vy();
        vtxZgen_lastDau = (MCcascade.template daughters_as<TMCParticle>().begin() + 1).vz();
        // V0
        ptGenV0 = MCv0.pt();
        prodVtxXgenV0 = MCv0.vx();
        prodVtxYgenV0 = MCv0.vy();
        prodVtxZgenV0 = MCv0.vz();
        vtxXgen_firstV0Dau = MCv0.template daughters_as<TMCParticle>().begin().vx();
        vtxYgen_firstV0Dau = MCv0.template daughters_as<TMCParticle>().begin().vy();
        vtxZgen_firstV0Dau = MCv0.template daughters_as<TMCParticle>().begin().vz();

        if (abs(MCcascade.pdgCode()) == 3312) { // Xi
          isTrueCasc = 1;
        } else {
          isTrueCasc = 0;
        }
        if (MCcascade.isPhysicalPrimary()) {
          source = 1;
        } else if (MCcascade.getProcess() == 4) { // from particle decay
          source = 2;
        } else if (MCcascade.fromBackgroundEvent()) {
          source = -1;
        } else {
          source = -2;
        }
        // fill cascade table
        fillCascMCTable(collision);

        // fill QA histos --> vertex position from daughters!
        histos.fill(HIST("hGenDecayVtxX_firstDau"), vtxXgen_firstDau);
        histos.fill(HIST("hGenDecayVtxY_firstDau"), vtxYgen_firstDau);
        histos.fill(HIST("hGenDecayVtxZ_firstDau"), vtxZgen_firstDau);
        histos.fill(HIST("hGenDecayVtxX_lastDau"), vtxXgen_lastDau);
        histos.fill(HIST("hGenDecayVtxY_lastDau"), vtxYgen_lastDau);
        histos.fill(HIST("hGenDecayVtxZ_lastDau"), vtxZgen_lastDau);

        histos.fill(HIST("hGenSource"), source);
        histos.fill(HIST("hChargeCounterCascDatas"), charge);
      }
    }
  }

  template <typename TCollision>
  void fillCascDataTable(TCollision const& collision)
  {
    rowCasc(collision.globalIndex(),
            pt, ptKF,
            massXi, massXiKF,
            massOmega, massOmegaKF,
            cascRad, cascRadKF,
            vtxX, vtxY, vtxZ, vtxXErr, vtxYErr, vtxZErr,
            vtxXKF, vtxXKF, vtxXKF, vtxXErrKF, vtxYErrKF, vtxZErrKF,
            dcaXYCascToPV, dcaXYCascToPVKF,
            dcaZCascToPV, dcaZCascToPVKF,
            dcaCascDaughters, dcaCascDaughtersKF,
            cascPointingAngle, cascPointingAngleKF,
            charge, 
            massLambda, massLambdaKF,
            V0Rad, V0RadKF,
            dcaV0Daughters, dcaV0DaughtersKF,
            dcaPosToPV, dcaPosToPVKF,
            dcaNegToPV, dcaNegToPVKF,
            dcaBachToPV, dcaBachToPVKF,
            v0PointingAngle, v0PointingAngleKF,
            isDCAfitter, isKF);
  }

  template <typename TCollision>
  void fillCascMCTable(TCollision const& collision)
  {
    rowCascMC(collision.globalIndex(),
              ptRec, ptRecKF, ptGen,
              massXi, massXiKF,
              massOmega, massOmegaKF,
              cascRad, cascRadKF,
              vtxXrec, vtxYrec, vtxZrec, vtxXrecErr, vtxYrecErr, vtxZrecErr,
              vtxXrecKF, vtxYrecKF, vtxZrecKF, vtxXrecErrKF, vtxYrecErrKF, vtxZrecErrKF,
              vtxXgen_firstDau, vtxYgen_firstDau, vtxZgen_firstDau,
              prodVtxXgen, prodVtxYgen, prodVtxZgen,
              dcaXYCascToPV, dcaXYCascToPVKF,
              dcaZCascToPV, dcaZCascToPVKF,
              dcaCascDaughters, dcaCascDaughtersKF,
              cascPointingAngle, cascPointingAngleKF,
              charge, 
              massLambda, massLambdaKF,
              V0Rad, V0RadKF,
              dcaV0Daughters, dcaV0DaughtersKF,
              dcaPosToPV, dcaPosToPVKF,
              dcaNegToPV, dcaNegToPVKF,
              dcaBachToPV, dcaBachToPVKF,
              v0PointingAngle, v0PointingAngleKF,
              isDCAfitter, isKF,
              isTrueCasc,
              source);
    LOG(info) << "CascMCTable filled!";
  }

  void processData(soa::Filtered<soa::Join<aod::Collisions, aod::EvSels>>::iterator const& collision, CascadesCrossLinked const& Cascades, soa::Join<aod::CascDatas, aod::CascCovs> const& CascDatas, soa::Join<aod::KFCascDatas, aod::KFCascCovs> const& KFCascDatas, aod::TracksIU const&)
  {
    /// Event selection
    histos.fill(HIST("hEventSelectionFlow"), 1.f);
    // select collisions in acceptance
    if (!(abs(collision.posZ()) < 10.)) return;
    histos.fill(HIST("hEventSelectionFlow"), 2.f);
   
    for (auto& cascade : Cascades) { // allows for cross-referencing everything

      // get charge from bachelor (unambiguous wrt to building)
      auto bachTrack = cascade.bachelor_as<aod::TracksIU>();
      if (bachTrack.sign() < 0) {
        charge = -1;
      } else {
        charge = +1;
      }
      histos.fill(HIST("hChargeCounter"), charge);

      // get cascade data and fill table
      getCascDatas(collision, cascade, CascDatas, KFCascDatas);
      if (cascade.has_cascData() || cascade.has_kfCascData()) {
        fillCascDataTable(collision);
      }
    } // end cascade loop
  } // end process
  PROCESS_SWITCH(kfStrangenessStudy, processData, "process data", true);

  void processMC(soa::Filtered<soa::Join<aod::Collisions, aod::EvSels>>::iterator const& collision, CascadesCrossLinked const& Cascades, CascDataLabeled const& CascDatas, KFCascDataLabeled const& KFCascDatas, aod::TracksIU const&, aod::McParticles const& particlesMC)
  {
    /// Event selection
    histos.fill(HIST("hEventSelectionFlow"), 1.f);
    // select collisions in acceptance
    if (!(abs(collision.posZ()) < 10.)) return;
    histos.fill(HIST("hEventSelectionFlow"), 2.f);

    for (auto& cascade : Cascades) {

      // get charge from bachelor (unambiguous wrt to building)
      auto bachTrack = cascade.bachelor_as<aod::TracksIU>();
      if (bachTrack.sign() < 0) {
        charge = -1;
      } else {
        charge = +1;
      }
      histos.fill(HIST("hChargeCounter"), charge);

      // get cascade data
      getCascDatas(collision, cascade, CascDatas, KFCascDatas);

      // ========== get cascade MC information ===========
      if (cascade.has_kfCascData() && cascade.has_cascData()) {
        LOG(info) << "Both fitters were successful!";
        auto cascdata = cascade.cascData_as<CascDataLabeled>();
        getCascMCdata(collision, cascdata, particlesMC);
      }
      if (cascade.has_kfCascData() && !cascade.has_cascData()) {
        LOG(info) << "Only KF was successful!";
        auto cascdata = cascade.kfCascData_as<KFCascDataLabeled>();
        getCascMCdata(collision, cascdata, particlesMC);
      }
      if (!cascade.has_kfCascData() && cascade.has_cascData()) {
        LOG(info) << "Only DCA fitter was successful!";
        auto cascdata = cascade.cascData_as<CascDataLabeled>();
        getCascMCdata(collision, cascdata, particlesMC);
      }
      
    } // end cascade loop
  } // end process
  PROCESS_SWITCH(kfStrangenessStudy, processMC, "process MC", false);

}; 

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<kfStrangenessStudy>(cfgc)};
}
