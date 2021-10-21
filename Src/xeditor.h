#ifndef XEDITOR_H
#define XEDITOR_H
#pragma once

#include <stdexcept>

//
// Key Includes
//
#include "xcore.h"
#include "../dependencies/xcore/dependencies/tracy/imgui/imgui.h"
#include "xGPU.h"

//
// Predefinitions 
//
namespace xeditor
{
}

namespace xeditor::document
{
    class base;
    class main;
}

namespace xeditor::command
{
    class system;
}

namespace xeditor::frame
{
    class base;
}

//
// Public headers
//
#include "xeditor_command.h"
#include "xeditor_document_base.h"
#include "xeditor_document_main.h"
#include "xeditor_plugins.h"
#include "xeditor_frame.h"
#include "xeditor_ui.h"

//
// Private headers
//
#include "details/xeditor_document_base_inline.h"
#include "details/xeditor_document_main_inline.h"
#include "details/xeditor_ui_inline.h"
#include "details/xeditor_frame_inline.h"


#endif