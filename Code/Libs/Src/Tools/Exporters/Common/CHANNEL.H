#ifndef __CHANNEL_H
#define __CHANNEL_H

#include "persistanim.h"
#include "persistchannel.h"

#define CHNL 		"Chnl"
#define SCRIPT 		"Script"
#define ANIMATION 	"Animation"

typedef PersistChannelHeader 		Header;

typedef struct
{
  int channel_count;
  PersistAnimChannelMapping *channel_list;
}Script;

typedef struct
{
  Header header;
  unsigned char *data;
}Channel;

typedef struct
{
  char name[PersistAnimCHANNEL_NAME_MAX];
  Channel channel;
  int data_size;
  int first_frame; // used for sequencing several channels in one script
  int last_frame;
}NamedChannel;

#endif
