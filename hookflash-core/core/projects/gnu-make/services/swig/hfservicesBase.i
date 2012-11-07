%module(directors="1") hfcoreBase
%import "stl.i"
%{
#include <hookflash/services/wrapper/hookflashTypes.h>
#include <hookflash/services/wrapper/hookflashEnums.h>
#include <hookflash/services/wrapper/internal/Helper.h>
#include <hookflash/services/wrapper/internal/RUDPSingleton.h>
#include <hookflash/services/wrapper/internal/RUDPListener.h>
#include <hookflash/services/wrapper/internal/RUDPMessaging.h>
#include <hookflash/services/wrapper/internal/RUDPICESocket.h>
#include <hookflash/services/wrapper/internal/RUDPICESocketSession.h>
#include <hookflash/services/wrapper/internal/FinderClient.h>
%}

%ignore hookflash::services::wrapper::internal::RUDPMessagingDelegateImpl;
%ignore hookflash::services::wrapper::internal::FinderClientDelegateImpl;


%feature("director") hookflash::services::wrapper::internal::RUDPSingleton;
%feature("director") hookflash::services::wrapper::internal::RUDPListenerDelegateCallback;
%feature("director") hookflash::services::wrapper::internal::RUDPListenerDelegate;
%feature("director") hookflash::services::wrapper::internal::RUDPListener;
%feature("director") hookflash::services::wrapper::internal::RUDPMessagingDelegateCallback;
%feature("director") hookflash::services::wrapper::internal::RUDPMessagingDelegate;
%feature("director") hookflash::services::wrapper::internal::RUDPMessaging;
%feature("director") hookflash::services::wrapper::internal::RUDPICESocket;
%feature("director") hookflash::services::wrapper::internal::RUDPICESocketSession;
%feature("director") hookflash::services::wrapper::internal::FinderClient;
%feature("director") hookflash::services::wrapper::internal::FinderClientDelegate;
%feature("director") hookflash::services::wrapper::internal::FinderClientDelegateCallback;



%include <../../../../../core/hookflash/services/wrapper/hookflashTypes.h>
%include <../../../../../core/hookflash/services/wrapper/hookflashEnums.h>
%include <../../../../../core/hookflash/services/wrapper/internal/Helper.h>
%include <../../../../../core/hookflash/services/wrapper/internal/RUDPSingleton.h>
%include <../../../../../core/hookflash/services/wrapper/internal/RUDPListener.h>
%include <../../../../../core/hookflash/services/wrapper/internal/RUDPMessaging.h>
%include <../../../../../core/hookflash/services/wrapper/internal/RUDPICESocket.h>
%include <../../../../../core/hookflash/services/wrapper/internal/RUDPICESocketSession.h>
%include <../../../../../core/hookflash/services/wrapper/internal/FinderClient.h>



#%template(vector_IGroupOpp_p) std::vector<hookflash::opp::wrapper::IGroupOpp*>;


