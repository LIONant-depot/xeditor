#ifndef XEDITOR_H
#define XEDITOR_H
#pragma once

#define XEDITOR_JOIN_STR(...) __VA_ARGS__

//
// System Includes
//
#include <stdexcept>

//
// 3rd Party Includes
//
#include "xcore.h"
#include "../dependencies/xcore/dependencies/tracy/imgui/imgui.h"
#include "xGPU.h"
#include "../Tools/xgpu_view.h"
#include "../dependencies/IconFontCppHeaders/IconsFontAwesome5.h"
#include "../dependencies/IconFontCppHeaders/IconsKenney.h"
#include "../dependencies/IconFontCppHeaders/IconsMaterialDesign.h"

//
// Predefinitions 
//
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

namespace xeditor::tab
{
    class base;
}

//
// Public headers
//
#include "xeditor_command.h"
#include "xeditor_document_base.h"
#include "xeditor_document_main.h"
#include "xeditor_tab.h"
#include "xeditor_frame.h"
#include "xeditor_ui.h"

//
// Private headers
//
#include "details/xeditor_document_base_inline.h"
#include "details/xeditor_document_main_inline.h"
#include "details/xeditor_ui_inline.h"
#include "details/xeditor_frame_inline.h"
#include "details/xeditor_tab_inline.h"

#endif