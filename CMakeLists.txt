################################################################################
# Package: DerivationFrameworkEoverP
################################################################################

# Declare the package name:
atlas_subdir( DerivationFrameworkEoverP )

# Find the needed external(s):
find_package( ROOT COMPONENTS Core RIO Hist Tree )
find_package(HepPDT REQUIRED)

atlas_install_python_modules( python/*.py )
atlas_install_joboptions( share/*.py )

atlas_add_component(DerivationFrameworkEoverP DerivationFrameworkEoverP/*.h src/*.cxx src/components/*.cxx
                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS} ${HEPPDT_INCLUDE_DIRS}
      		       LINK_LIBRARIES  ${ROOT_LIBRARIES}  ${HEPPDT_LIBRARIES}
                   ${release_libs} GaudiKernel xAODEventInfo TrkExInterfaces CaloUtilsLib TileEvent AthenaBaseComps
                   RecoToolInterfaces xAODMuon JpsiUpsilonToolsLib EventPrimitives xAODBPhysLib DerivationFrameworkInterfaces
                   PRIVATE_LINK_LIBRARIES InDetV0FinderLib
                   TrkVertexAnalysisUtilsLib TrkVKalVrtFitterLib CaloSimEvent MCTruthClassifierLib xAODTruth 
)
