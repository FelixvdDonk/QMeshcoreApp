#ifndef QMESHCORE_PLUGIN_H
#define QMESHCORE_PLUGIN_H

// This header is used by the Qt type registration system
// Include all headers that define QML_ELEMENT types

#include "MeshCoreConstants.h"
#include "MeshCoreDevice.h"

#include "types/Contact.h"
#include "types/SelfInfo.h"
#include "types/DeviceInfo.h"
#include "types/ChannelInfo.h"
#include "types/ContactMessage.h"
#include "types/ChannelMessage.h"
#include "types/RepeaterStats.h"
#include "types/Advert.h"
#include "types/Packet.h"
#include "types/TraceData.h"
#include "types/TelemetryData.h"

#include "models/ContactModel.h"
#include "models/ChannelModel.h"
#include "models/MessageModel.h"

#endif // QMESHCORE_PLUGIN_H
