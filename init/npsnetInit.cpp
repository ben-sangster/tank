#include "npsnetInit.h"
#include <dmzApplication.h>
#include <dmzAppShellExt.h>
#include <dmzFoundationCommandLine.h>
#include <dmzFoundationXMLUtil.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimeVersion.h>
#include <dmzTypesHashTableStringTemplate.h>

#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QCloseEvent>

using namespace dmz;

namespace {

typedef HashTableStringTemplate<String> FileTable;
typedef HashTableStringTemplate<Config> ConfigTable;

static void
local_populate_color_table (
      AppShellInitStruct &init,
      NPSNetInit &ci,
      ConfigTable &colorTable) {

   Config colorList;

   if (init.manifest.lookup_all_config ("color.type", colorList)) {

      ConfigIterator it;
      Config color;

      while (colorList.get_next_config (it, color)) {

         String value = config_to_string ("text", color);

         if (value) {

            ci.ui.colorCombo->addItem (value.get_buffer ());
            Config *ptr = new Config (color);

            if (ptr && !colorTable.store (value, ptr)) {

               delete ptr; ptr = 0;
            }
         }
      }
   }
}


static void
local_populate_resolution_table (
      AppShellInitStruct &init,
      NPSNetInit &ci,
      ConfigTable &rezTable) {

   Config rezList;

   if (init.manifest.lookup_all_config ("screen.resolution", rezList)) {

      ConfigIterator it;
      Config rez;

      while (rezList.get_next_config (it, rez)) {

         String value = config_to_string ("text", rez);

         if (value) {

            ci.ui.resolutionCombo->addItem (value.get_buffer ());

            Config *ptr = new Config (rez);

            if (!rezTable.store (value, ptr) && ptr) {

               delete ptr; ptr = 0;
            }
         }
      }
   }
}


static void
local_add_config (const String &Scope, AppShellInitStruct &init) {

   Config configList;

   if (init.manifest.lookup_all_config (Scope, configList)) {

      ConfigIterator it;
      Config config;

      while (configList.get_next_config (it, config)) {

         const String Value = config_to_string ("file", config);

         if (Value) { init.files.append_arg (Value); }
      }
   }
}


static void
local_setup_resolution (
      AppShellInitStruct &init,
      NPSNetInit &ci,
      ConfigTable &rezTable) {

   Config global;

   init.app.get_global_config (global);

   Config *ptr = rezTable.lookup (qPrintable (ci.ui.resolutionCombo->currentText ()));

   if (ptr) {

      Config attrList;

      ptr->lookup_all_config ("attribute", attrList);

      ConfigIterator it;
      Config attr;

      while (attrList.get_next_config (it, attr)) {

         const String Scope = config_to_string ("scope", attr);
         const String Value = config_to_string ("value", attr);

         if (Scope && Value) { global.store_attribute (Scope, Value); }
      }
   }

   String ScreenScope = config_to_string ("screen.scope", init.manifest);

   if (ScreenScope) {

      const String Screen = qPrintable (ci.ui.screenBox->cleanText ());

      global.store_attribute (ScreenScope, Screen);
   }
}


static void
local_set_port (const Int32 Port, AppShellInitStruct &init) {

   Config global;

   init.app.get_global_config (global);

   String PortScope = config_to_string (
      "port.scope",
      init.manifest,
      "dmz.dmzNetModulePacketIOHawkNL.socket.port");

   if (PortScope) {

      global.store_attribute (PortScope, String::number (Port));
   }
}

};

NPSNetInit::NPSNetInit (AppShellInitStruct &theInit) : init (theInit), _start (False) {

   ui.setupUi (this);
}


NPSNetInit::~NPSNetInit () {

}


void
NPSNetInit::on_buttonBox_accepted () {

   _start = True;
   close ();
}


void
NPSNetInit::on_buttonBox_rejected () {

   close ();
}


void
NPSNetInit::on_buttonBox_helpRequested () {

   const String UrlValue =
      config_to_string ("help.url", init.manifest, "http://dmzdev.org/wiki/npsnet");

   if (UrlValue) {

      QUrl Url (UrlValue.get_buffer ());

      QDesktopServices::openUrl (Url);
   }
}

void
NPSNetInit::closeEvent (QCloseEvent * event) {

   if (!_start) {

      init.app.quit ("Cancel Button Pressed");
   }

   event->accept ();
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL void
dmz_init_tankdemo (AppShellInitStruct &init) {

   NPSNetInit ci (init);

   if (init.VersionFile) {

      Version version;

      if (xml_to_version (init.VersionFile, version, &init.app.log)) {

         QString vs = ci.windowTitle ();
         vs += " (v";
         const String Tmp = version.get_version ().get_buffer ();
         if (Tmp) { vs += Tmp.get_buffer (); }
         else { vs += "Unknown"; }
         vs += ")";

         ci.setWindowTitle (vs);
      }
   }

   ConfigTable colorTable;

   local_populate_color_table (init, ci, colorTable);

   ConfigTable rezTable;

   local_populate_resolution_table (init, ci, rezTable);

   ci.show ();
   ci.raise ();

   while (ci.isVisible ()) {

      QApplication::sendPostedEvents (0, -1);
      QApplication::processEvents (QEventLoop::WaitForMoreEvents);
   }

   if (init.app.is_running ()) {

      local_add_config ("config", init);

      Config *colorPtr = colorTable.lookup (
         qPrintable (ci.ui.colorCombo->currentText ()));

      if (colorPtr) {

         Config fileList;

         if (colorPtr->lookup_all_config ("config", fileList)) {

            ConfigIterator it;
            Config file;

            while (fileList.get_next_config (it, file)) {

               init.files.append_arg (config_to_string ("file", file));
            }
         }
      }

      CommandLine cl;
      cl.add_args (init.files);
      init.app.process_command_line (cl);

      if (!init.app.is_error ()) {

         local_setup_resolution (init, ci, rezTable);

         local_set_port (ci.ui.portBox->value (), init);
      }
   }
}

};
