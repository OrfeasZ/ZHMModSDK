#pragma once

namespace Scaleform {
    namespace GFx {
        namespace AS3 {
            class FlashUI {
            public:
                enum class OutputMessageType {
                    Output_Message = 0,
                    Output_Error = 1,
                    Output_Warning = 2,
                    Output_Action = 3
                };
            };

            class MovieRoot;
        }
    }
};