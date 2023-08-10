/*
  Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
*/

///////////////////////////////////////////////////////////////////
// Reco_mumu.h
///////////////////////////////////////////////////////////////////

#ifndef DERIVATIONFRAMEWORKEOP__Reco_mumu_H
#define DERIVATIONFRAMEWORKEOP__Reco_mumu_H

#include <string>

#include "AthenaBaseComps/AthAlgTool.h"
#include "GaudiKernel/ToolHandle.h"
#include "DerivationFrameworkInterfaces/IAugmentationTool.h"
#include "JpsiUpsilonTools/JpsiFinder.h"
#include "JpsiUpsilonTools/PrimaryVertexRefitter.h"
#include "BeamSpotConditionsData/BeamSpotData.h"
#include "TrkVertexAnalysisUtils/V0Tools.h"

/** forward declarations
 */
//namespace Trk {
  //class V0Tools;
//}

/** THE reconstruction tool
 */
namespace DerivationFramework {

  class Reco_mumu : public AthAlgTool, public IAugmentationTool {
  public: 
    Reco_mumu(const std::string& t, const std::string& n, const IInterface* p);

    StatusCode initialize();
    StatusCode finalize();
      
    virtual StatusCode addBranches() const;
      
  private:
    /** tools
     */
    ToolHandle<Trk::V0Tools>                    m_v0Tools;
    ToolHandle<Analysis::JpsiFinder>            m_jpsiFinder;
    ToolHandle<Analysis::PrimaryVertexRefitter> m_pvRefitter;
    SG::ReadCondHandleKey<InDet::BeamSpotData> m_beamSpotKey { this, "BeamSpotKey", "BeamSpotData", "SG key for beam spot" };
      
    /** job options
     */
    std::string m_outputVtxContainerName;
    std::string m_pvContainerName;
    std::string m_refPVContainerName;
    bool        m_refitPV;
    int         m_PV_max;
    int         m_DoVertexType;
    size_t      m_PV_minNTracks;
    bool        m_do3d;
    bool        m_checkCollections;
    std::vector<std::string> m_CollectionsToCheck;
    SG::ReadHandleKey<xAOD::VertexContainer> m_pvContainerKey{this,"PVContainerName", "PrimaryVertices"};
    SG::WriteHandleKey<xAOD::VertexContainer> m_refContainerKey{this, "RefPVContainerName","RefittedPrimaryVertices" };
    SG::WriteHandleKey<xAOD::VertexContainer> m_outContainerKey{this, "OutputVtxContainerName", "OniaCandidates"};
  }; 
}

#endif // _Reco_mumu_H
