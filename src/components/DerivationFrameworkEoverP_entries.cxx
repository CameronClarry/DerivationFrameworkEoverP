#include "GaudiKernel/DeclareFactoryEntries.h"
#include "DerivationFrameworkEoverP/TrackCaloDecorator.h"
#include "DerivationFrameworkEoverP/Reco_mumu.h"

using namespace DerivationFramework;

DECLARE_TOOL_FACTORY( TrackCaloDecorator )
DECLARE_TOOL_FACTORY( Reco_mumu )

DECLARE_FACTORY_ENTRIES( DerivationFrameworkEoverP ) 
{
  DECLARE_TOOL( TrackCaloDecorator );
  DECLARE_TOOL( Reco_mumu );
}
