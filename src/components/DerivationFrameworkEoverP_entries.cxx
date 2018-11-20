#include "GaudiKernel/DeclareFactoryEntries.h"
#include "DerivationFrameworkEoverP/TrackCaloDecorator.h"
#include "DerivationFrameworkEoverP/Reco_mumu.h"
#include "DerivationFrameworkEoverP/Select_onia2mumu.h"

using namespace DerivationFramework;

DECLARE_TOOL_FACTORY( TrackCaloDecorator )
DECLARE_TOOL_FACTORY( Reco_mumu )
DECLARE_TOOL_FACTORY( Select_onia2mumu )

DECLARE_FACTORY_ENTRIES( DerivationFrameworkEoverP ) 
{
  DECLARE_TOOL( TrackCaloDecorator );
  DECLARE_TOOL( Reco_mumu );
  DECLARE_TOOL( Select_onia2mumu );
}
