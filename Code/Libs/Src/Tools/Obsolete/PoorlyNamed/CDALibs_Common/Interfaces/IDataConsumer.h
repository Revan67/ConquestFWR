// IDataConsumer
//
//
//

#ifndef IDataConsumer_H
#define IDataConsumer_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_IDataConsumer = "IDataConsumer";

dacom_interface( IDataConsumer )
{
	DACOM_INTERFACE_METHOD( SetDataProducer,	( IDAComponent *producer ));
	DACOM_INTERFACE_METHOD( GetDataProducer,	( IDAComponent **out_producer ));
};

#endif
