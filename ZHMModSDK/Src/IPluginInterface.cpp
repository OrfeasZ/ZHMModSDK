#include "IPluginInterface.h"
#include <implot.h>

void SetImPlotContext(ImPlotContext* p_Context)
{
    ImPlot::SetCurrentContext(p_Context);
}