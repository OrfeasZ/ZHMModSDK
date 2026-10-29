#include <Glacier/ZConfigCommand.h>
#include <Glacier/Hash.h>

#include "Functions.h"
#include "Util/StringUtils.h"

ZConfigCommand* ZConfigCommand::Get(ZString p_CommandName) {
    return Functions::ZConfigCommand_GetConfigCommand->Call(
        Hash::Fnv1a(
            Util::StringUtils::ToLowerCase(p_CommandName.c_str()).c_str()
        )
    );
}