# include "Sniffer.h"

namespace sniffer
{
	void RegisterForAllEvents()
	{
		EquipEventHandler::GetSingleton()->Register();
		ActorItemEquippedEventHandler::GetSingleton()->Register();
	}
}