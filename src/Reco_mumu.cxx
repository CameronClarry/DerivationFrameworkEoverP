/*
  Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
*/

/////////////////////////////////////////////////////////////////
// Reco_mumu.cxx
///////////////////////////////////////////////////////////////////
// Author: Daniel Scheirich <daniel.scheirich@cern.ch>
// Based on the Integrated Simulation Framework
//
// Basic Jpsi->mu mu derivation example

#include "DerivationFrameworkEoverP/Reco_mumu.h"

#include "xAODTracking/VertexContainer.h"
#include "xAODTracking/VertexAuxContainer.h"
#include "TrkVertexAnalysisUtils/V0Tools.h"
#include "DerivationFrameworkEoverP/BPhysPVTools.h"


namespace DerivationFramework {

  Reco_mumu::Reco_mumu(const std::string& t,
		       const std::string& n,
		       const IInterface* p) : 
    AthAlgTool(t,n,p),
    m_v0Tools("Trk::V0Tools"),
    m_jpsiFinder("Analysis::JpsiFinder")
  {
    declareInterface<DerivationFramework::IAugmentationTool>(this);
    
    // Declare tools    
    declareProperty("V0Tools"   , m_v0Tools);
    declareProperty("JpsiFinder", m_jpsiFinder);
    
    // Declare user-defined properties
    declareProperty("OutputVtxContainerName", m_outputVtxContainerName = "OniaCandidates");
    declareProperty("PVContainerName"       , m_pvContainerName        = "PrimaryVertices");
    declareProperty("RefPVContainerName"    , m_refPVContainerName     = "RefittedPrimaryVertices");
    declareProperty("RefitPV"               , m_refitPV                = false);
    declareProperty("MaxPVrefit"            , m_PV_max                 = 1);
    declareProperty("DoVertexType"          , m_DoVertexType           = 1);
    // minimum number of tracks for PV to be considered for PV association
    declareProperty("MinNTracksInPV"        , m_PV_minNTracks          = 0);
    declareProperty("Do3d"                  , m_do3d                   = false);
    declareProperty("CheckCollections"      , m_checkCollections       = false);
    declareProperty("CheckVertexContainers" , m_CollectionsToCheck);
  }

  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
  
  StatusCode Reco_mumu::initialize()
  {
  
    ATH_MSG_DEBUG("in initialize()");
 
    // retrieve V0 tools
    CHECK( m_v0Tools.retrieve() );
    
    // get the JpsiFinder tool
    CHECK( m_jpsiFinder.retrieve() );
     
    // get the PrimaryVertexRefitter tool
    CHECK( m_pvRefitter.retrieve() );

    // Get the beam spot service
    CHECK( m_beamSpotKey.initialize() );

    ATH_CHECK(m_pvContainerKey.initialize());
    ATH_CHECK(m_outContainerKey.initialize());
    ATH_CHECK(m_refContainerKey.initialize(m_refitPV));
    return StatusCode::SUCCESS;
    
  }
  
  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

  StatusCode Reco_mumu::finalize()
  {
    // everything all right
    return StatusCode::SUCCESS;
  }

  // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
  
  StatusCode Reco_mumu::addBranches() const
  {
    bool callJpsiFinder = true;
    const EventContext& ctx = Gaudi::Hive::currentContext();
    if(m_checkCollections) {
      for(const auto &str : m_CollectionsToCheck){
	const xAOD::VertexContainer*    vertContainer = nullptr;
	ATH_CHECK( evtStore()->retrieve(vertContainer, str) );
	if(vertContainer->size() == 0) {
	  callJpsiFinder = false;
	  ATH_MSG_DEBUG("Container VertexContainer (" << str << ") is empty");
	  break;//No point checking other containers
	}/*else{
            callJpsiFinder = true;
            ATH_MSG_DEBUG("Container VertexContainer (" << str << ") has events N= " << vertContainer->size());
	    }*/
      }
    }

    // Jpsi container and its auxilliary store
    std::unique_ptr<xAOD::VertexContainer>    vtxContainer = std::make_unique<xAOD::VertexContainer>();
    std::unique_ptr<xAOD::VertexAuxContainer> vtxAuxContainer = std::make_unique<xAOD::VertexAuxContainer>();
    vtxContainer->setStore(vtxAuxContainer.get());

    std::unique_ptr<xAOD::VertexContainer>    refPvContainer =std::make_unique<xAOD::VertexContainer>();
    std::unique_ptr<xAOD::VertexAuxContainer> refPvAuxContainer = std::make_unique<xAOD::VertexAuxContainer>();
    refPvContainer->setStore(refPvAuxContainer.get());
    
    if(callJpsiFinder) {
      //----------------------------------------------------
      // call Jpsi finder
      //----------------------------------------------------
      if( !m_jpsiFinder->performSearch(ctx, *vtxContainer).isSuccess() ) {
        ATH_MSG_FATAL("Jpsi finder (" << m_jpsiFinder << ") failed.");
        return StatusCode::FAILURE;
      }

      //----------------------------------------------------
      // retrieve primary vertices
      //----------------------------------------------------
      SG::ReadHandle<xAOD::VertexContainer>    pvContainer{m_pvContainerKey,ctx};
      if (!pvContainer.isValid()){
        ATH_MSG_FATAL("Failed to retrive "<<m_pvContainerKey.fullKey());
        return StatusCode::FAILURE;
      }

      //----------------------------------------------------
      // Try to retrieve refitted primary vertices
      //----------------------------------------------------

      // Give the helper class the ptr to v0tools and beamSpotsSvc to use
      SG::ReadCondHandle<InDet::BeamSpotData> beamSpotHandle { m_beamSpotKey, ctx };
      if(not beamSpotHandle.isValid()) ATH_MSG_ERROR("Cannot Retrieve " << m_beamSpotKey.key() );
      BPhysPVTools helper(&(*m_v0Tools), beamSpotHandle.cptr());
      helper.SetMinNTracksInPV(m_PV_minNTracks);
      helper.SetSave3d(m_do3d);

      if(m_refitPV){ 
        if(vtxContainer->size() >0){
          StatusCode SC = helper.FillCandwithRefittedVertices(vtxContainer.get(),  pvContainer.cptr(), refPvContainer.get(), &(*m_pvRefitter) , m_PV_max, m_DoVertexType);
          if(SC.isFailure()){
            ATH_MSG_FATAL("refitting failed - check the vertices you passed");
            return SC;
          }
        }
      }else{
        if(vtxContainer->size() >0)CHECK(helper.FillCandExistingVertices(vtxContainer.get(), pvContainer.cptr(), m_DoVertexType));
      }
    
      //----------------------------------------------------
      // save in the StoreGate
      //----------------------------------------------------
      SG::WriteHandle<xAOD::VertexContainer> out_handle{m_outContainerKey, ctx};
      ATH_CHECK(out_handle.record(std::move(vtxContainer), std::move(vtxAuxContainer)));
      if(m_refitPV) {
        SG::WriteHandle<xAOD::VertexContainer> refitHandle{m_refContainerKey,ctx};
        ATH_CHECK(refitHandle.record(std::move(refPvContainer), std::move(refPvAuxContainer)));
      }
    }    

    return StatusCode::SUCCESS;
  }  
}
