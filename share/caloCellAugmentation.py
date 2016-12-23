########################################
# Derivation Framework
########################################
from AthenaCommon import CfgMgr

# DerivationJob is COMMON TO ALL DERIVATIONS
DerivationFrameworkJob = CfgMgr.AthSequencer("AthAlgSeq2")

# Set up stream auditor
from AthenaCommon.AppMgr import ServiceMgr as svcMgr
if not hasattr(svcMgr, 'DecisionSvc'):
        svcMgr += CfgMgr.DecisionSvc()
svcMgr.DecisionSvc.CalcStats = True

streamName = "CALO"

########################################
# Tools
########################################

# Track Calo Decorator
from EoverPxAOD.EoverPxAODConf import DerivationFramework__TrackCaloDecorator
CaloDeco = DerivationFramework__TrackCaloDecorator(name = "TrackCaloDecorator",
                                                   TrackContainer = "InDetTrackParticles",
                                                   CaloClusterContainer = "CaloCalTopoClusters",
                                                   DecorationPrefix = streamName,
                                                   OutputLevel = DEBUG)

ToolSvc += CaloDeco

########################################
# Augmentation
########################################

from DerivationFrameworkCore.DerivationFrameworkCoreConf import DerivationFramework__CommonAugmentation

DerivationFrameworkJob += CfgMgr.DerivationFramework__CommonAugmentation("TrackCaloDecorator_KERN",
                                                                         AugmentationTools = [CaloDeco],
                                                                         OutputLevel = WARNING)

topSequence += DerivationFrameworkJob

########################################
# Trigger Metadata
########################################

ToolSvc += CfgMgr.xAODMaker__TriggerMenuMetaDataTool(
        "TriggerMenuMetaDataTool" )
svcMgr.MetaDataSvc.MetaDataTools += [ ToolSvc.TriggerMenuMetaDataTool ]

########################################
# Output
########################################

exclusionVars_TrackParticles='-caloExtension.-cellAssociation.-trackParameterCovarianceMatrices.-parameterX.-parameterY\
.-parameterZ.-parameterPX.-parameterPY.-parameterPZ.-parameterPosition'

# Build output stream
from OutputStreamAthenaPool.MultipleStreamManager import MSMgr
fileName   = "TrackCaloDxAOD.pool.root"
TestStream = MSMgr.NewPoolRootStream( streamName, fileName)
# General info
TestStream.AddItem("xAOD::EventInfo#*")
TestStream.AddItem("xAOD::EventAuxInfo#*")
# Tracking info
TestStream.AddItem("xAOD::TrackParticleContainer#InDetTrackParticles")
TestStream.AddItem("xAOD::TrackParticleAuxContainer#InDetTrackParticlesAux."+exclusionVars_TrackParticles)
TestStream.AddItem("xAOD::VertexContainer#PrimaryVertices")
TestStream.AddItem("xAOD::VertexAuxContainer#PrimaryVerticesAux.-vxTrackAtVertex")

########################################
# Trigger Information
########################################
TestStream.AddMetaDataItem("xAOD::TriggerMenuContainer#TriggerMenu")
TestStream.AddMetaDataItem("xAOD::TriggerMenuAuxContainer#TriggerMenuAux.")

TestStream.AddItem("xAOD::TrigDecision#xTrigDecision")
TestStream.AddItem("xAOD::TrigDecisionAuxInfo#xTrigDecisionAux.")
TestStream.AddItem("xAOD::TrigConfKeys#TrigConfKeys")

