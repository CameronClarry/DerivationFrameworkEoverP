#include "GaudiKernel/DeclareFactoryEntries.h"
#include "DerivationFrameworkEoverP/TrackCaloDecorator.h"

using namespace DerivationFramework;

DECLARE_TOOL_FACTORY( TrackCaloDecorator )

DECLARE_FACTORY_ENTRIES( DerivationFrameworkEoverP ) 
{
  DECLARE_TOOL( TrackCaloDecorator );
}
