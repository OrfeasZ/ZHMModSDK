#include <Glacier/ZConfigCommand.h>

#include "Functions.h"
#include "Util/StringUtils.h"
#include "Util/HashingUtils.h"

ZConfigCommand* ZConfigCommand::Get(ZString p_CommandName) {
    return Functions::ZConfigCommand_GetConfigCommand->Call(
        Util::HashingUtils::FNV1a(
            Util::StringUtils::ToLowerCase(p_CommandName.c_str()).c_str()
        )
    );
}
