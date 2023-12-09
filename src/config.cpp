#include <cassert>
#include <iostream>
#include <typeinfo>
#include <sstream>

#include <libgen.h>

#include "config.h"
#include "shim.h"
#include "utils.h"

using namespace project;
using namespace std;

namespace project {

///////////////////////////////////////////////////////////////////////////////
//
// configurator
// base class of configuration file reader/writter
//
///////////////////////////////////////////////////////////////////////////////

configurator::configurator() : m_ver(enum_ver_1) {}

configurator::~configurator() {}

bool configurator::load_config(const string &fn) {
  try {
    m_ifn = fn;
    reset_error();
    setIncludeDir(util_extract_path(fn).c_str());
    readFile(m_ifn.c_str());

    if (exists("ver")) {
      m_ver = (int)lookup("ver");
    }

    return true;
  } catch (const exception_io &ei) {
    ostringstream oss;
    oss << "failed to load config file " << m_ifn << ", " << ei.what();
    m_error = oss.str();
    return false;
  } catch (const exception_parsing &ep) {
    ostringstream oss;
    oss << "failed to parse config file at " << ep.getFile() << ":"
        << ep.getLine() << ", " << ep.getError();
    m_error = oss.str();
    return false;
  }
}

bool configurator::save_config(const string &fn) {
  try {
    m_ofn = fn;
    reset_error();
    setIncludeDir(util_extract_path(fn).c_str());
    result_cfg.writeFile(fn.c_str());
    return true;
  } catch (const exception_io &ei) {
    ostringstream oss;
    oss << "failed to load config file " << m_ofn << ", " << ei.what();
    m_error = oss.str();
    return false;
  }
}

void configurator::reset_result_cfg() {
    settings &root = result_cfg.getRoot();
    while (root.getLength() > 0)
      root.remove((unsigned int)0);
}

///////////////////////////////////////////////////////////////////////////////
//
// meta_cfg
// class of generating schema of configuration file
//
///////////////////////////////////////////////////////////////////////////////

void meta_cfg::traverse(settings &set) {
  static int s_nested = 0;
  static const char *s_types[] = {
    "none",
    "int",
    "int64",
    "float",
    "string",
    "bool",
    "group",
    "array",
    "list"
  };

  cout << string(s_nested * 2, ' ') << (set.getName() ? set.getName() : "(null)")
       << ", " << s_types[set.getType()]
       << ", index " << set.getIndex()
       << ", path " << set.getPath()
       << endl;

  if (set.isAggregate()) {
    ++s_nested;
    for (settings::iterator it = set.begin(); it != set.end(); ++it) {
      if (it->isAggregate()) {
          traverse(*it);
      } else {
        cout << string(s_nested * 2, ' ') << (it->getName() ? it->getName() : "(null)")
            << ", " << s_types[it->getType()]
            << ", index " << it->getIndex()
            << endl;
      }
    }
    --s_nested;
  }
}

bool meta_cfg::parse_config() {
  try {
    reset_error();
    traverse(getRoot());
    return true;
  }
  catch (const exception_setting &es) {
    ostringstream oss;
    oss << "exception occurred during parsing settings, " << es.what() << ", " << es.getPath();
    m_error = oss.str();
    return false;
  }
}

bool meta_cfg::migrate_config(const string &, const string &) {
  return true;
}

bool meta_cfg::build_config() {
  return true;
}

bool meta_cfg::save_config(const string &fn) {
  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// app_cfg
// class of application configuration file reader/writter
//
///////////////////////////////////////////////////////////////////////////////

bool app_cfg::build_config()
{
  return true;
}

bool app_cfg::parse_config() {
  try {
    // find app config
    shim &shi = shim::instance();
    app_config_ptr ac =
        std::dynamic_pointer_cast<app_config>(shi.find_config("app_config"));
    assert(ac != nullptr);
    ac->unsubscribe(&shi);

    reset_error();
    const settings &as = getRoot()["app"];

    // optional
    if (as.exists("thread_pool_size"))
      ac->set_thread_pool_size((unsigned)as.lookup("thread_pool_size"));
    if (as.exists("max_req_per_msg"))
      ac->set_max_req_per_msg((unsigned)as.lookup("max_req_per_msg"));
    if (as.exists("flush_interval_ms"))
      ac->set_flush_interval_ms((unsigned)as.lookup("flush_interval_ms"));
    if (as.exists("log_path"))
      ac->set_log_path((const char *)as.lookup("log_path"));
    if (as.exists("sas_log_level"))
      ac->set_sas_log_level((const char *)as.lookup("sas_log_level"));
    if (as.exists("dev_log_level"))
      ac->set_dev_log_level((const char *)as.lookup("dev_log_level"));
    if (as.exists("con_log_level"))
      ac->set_con_log_level((const char *)as.lookup("con_log_level"));

    shi.insert_config(ac);
    return true;
  } catch (const exception_setting &es) {
    ostringstream oss;
    oss << "exception occurred during parsing settings, " << es.what() << ", "
        << es.getPath();
    m_error = oss.str();
    return false;
  }
}

bool app_cfg::migrate_config(const string &ifn, const string &ofn) {
  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// ap_cfg
// class of ap configuration file reader/writter
//
///////////////////////////////////////////////////////////////////////////////

bool ap_cfg::parse_config() {
  try {
    // shim layer instance
    shim &sh = shim::instance();
    reset_error();

    const settings &ds = getRoot()["sites"];
    cout << "sites num = " << ds.getLength() << endl;
    for (const auto &d : ds) {
      // new site config
      site_config_ptr dc = site_config::create();
      // mandatory
      const char *dc_name = d.lookup("name");
      dc->set_name(dc_name);
      // optional
      if (d.exists("tls.dns_cache"))
        dc->set_dns_cache((const char *)d.lookup("tls.dns_cache"));
      if (d.exists("tls.dns_interval"))
        dc->set_dns_interval((long)d.lookup("tls.dns_interval"));
      if (d.exists("tls.local_port"))
        dc->set_local_port((int)d.lookup("tls.local_port"));
      if (d.exists("tls.maxage_conn"))
        dc->set_maxage_conn((long)d.lookup("tls.maxage_conn"));
      if (d.exists("tls.conn_timeout"))
        dc->set_conn_timeout((long)d.lookup("tls.conn_timeout"));
      if (d.exists("tls.ip_resolve"))
        dc->set_ip_resolve((long)d.lookup("tls.ip_resolve"));
      if (d.exists("tls.interface"))
        dc->set_interface((const char *)d.lookup("tls.interface"));
      if (d.exists("tls.tcp_keepalive_idle"))
        dc->set_tcp_keepalive_idle((long)d.lookup("tls.tcp_keepalive_idle"));
      if (d.exists("tls.tcp_keepalive_interval"))
        dc->set_tcp_keepalive_interval(
            (long)d.lookup("tls.tcp_keepalive_interval"));
      if (d.exists("tls.tls_debug"))
        dc->set_tls_debug((bool)d.lookup("tls.tls_debug"));

      dc->get_dns_cache();
      // ostringstream ossd;
      // dc->dump(ossd);
      // cout << ossd.str() << endl;
      sh.insert_config(dc);

      const settings &ts = d["buildings"];
      cout << "  building num = " << ts.getLength() << endl;
      for (const auto &t : ts) {
        // new building config
        building_config_ptr tc = building_config::create();
        // mandatory
        const char *tc_name = (const char *)t.lookup("name");
        const char *site_name = (const char *)t.lookup("site_name");
        const char *sas_url = (const char *)t.lookup("sas_url");
        const char *user_id = (const char *)t.lookup("user_id");
        const char *ca_path = (const char *)t.lookup("ca_path");
        tc->set_name(tc_name);
        tc->set_site_name(site_name);
        tc->set_sas_url(sas_url);
        tc->set_user_id(user_id);
        tc->set_ca_path(ca_path);
        // optional
        if (t.exists("root_ca"))
          tc->set_root_ca((const char *)t.lookup("root_ca"));
        if (t.exists("sas_crl"))
          tc->set_sas_crl((const char *)t.lookup("sas_crl"));
        // ostringstream osst;
        // tc->dump(osst);
        // cout << osst.str() << endl;
        sh.insert_config(tc);

        const settings &cs = t["aps"];
        cout << "  ap num = " << cs.getLength() << endl;
        for (const auto &c : cs) {
          // new ap config
          ap_config_ptr cc = ap_config::create();
          // mandatory
          const char *site_name = (const char *)c.lookup("site_name");
          const char *building_name = (const char *)c.lookup("building_name");
          bool admin_state = (bool)c.lookup("admin_state");
          bool single_step = (bool)c.lookup("single_step");
          unsigned central_freq_khz = (unsigned)c.lookup("central_freq_khz");
          unsigned radio_bandwidth_mhz =
              (unsigned)c.lookup("radio_bandwidth_mhz");
          const char *fcc_id = (const char *)c.lookup("fcc_id");
          const char *serial_number = (const char *)c.lookup("serial_number");
          string ap_name = string(fcc_id) + ":" + string(serial_number);
          const char *category = (const char *)c.lookup("category");
          const char *call_sign = (const char *)c.lookup("call_sign");
          const settings &mcs = c["meas_capabilities"];
          list<string> meas_caps;
          for (const auto &m : mcs)
            meas_caps.push_back((const char *)m);
          const char *radio_technology =
              (const char *)c.lookup("radio_technology");
          const char *vendor = (const char *)c.lookup("vendor");
          const char *model = (const char *)c.lookup("model");
          const char *software_version =
              (const char *)c.lookup("software_version");
          const char *hardware_version =
              (const char *)c.lookup("hardware_version");
          const char *firmware_version =
              (const char *)c.lookup("firmware_version");
          int eirp_capability = (int)c.lookup("eirp_capability");
          double latitude = (double)c.lookup("latitude");
          double longitude = (double)c.lookup("longitude");
          double height = (double)c.lookup("height");
          const char *height_type = (const char *)c.lookup("height_type");
          double horizontal_accuracy = (double)c.lookup("horizontal_accuracy");
          double vertical_accuracy = (double)c.lookup("vertical_accuracy");
          bool indoor_site = (bool)c.lookup("indoor_site");
          int antenna_azimuth = (int)c.lookup("antenna_azimuth");
          int antenna_downtilt = (int)c.lookup("antenna_downtilt");
          int antenna_gain = (int)c.lookup("antenna_gain");
          int antenna_beamwidth = (int)c.lookup("antenna_beamwidth");
          const char *antenna_model = (const char *)c.lookup("antenna_model");
          const settings &gs = c["groups"];
          list<string> gts, gis;
          for (const auto &g : gs) {
            gts.push_back((const char *)g.lookup("type"));
            gis.push_back((const char *)g.lookup("id"));
          }
          const char *protected_header =
              (const char *)c.lookup("protected_header");
          const char *encoded_cpi_signed_data = (const char *)c.lookup(
              "encoded_cpi_signed_data");
          const char *digital_signature =
              (const char *)c.lookup("digital_signature");
          cc->set_site_name(site_name);
          cc->set_building_name(building_name);
          cc->set_admin_state(admin_state);
          cc->set_single_step(single_step);
          cc->set_central_freq_khz(central_freq_khz);
          cc->set_radio_bandwidth_mhz(radio_bandwidth_mhz);
          cc->set_name(ap_name);
          cc->set_fcc_id(fcc_id);
          cc->set_serial_number(serial_number);
          cc->set_category(category);
          cc->set_call_sign(call_sign);
          cc->set_meas_capabilities(meas_caps);
          cc->set_radio_technology(radio_technology);
          cc->set_vendor(vendor);
          cc->set_model(model);
          cc->set_software_version(software_version);
          cc->set_hardware_version(hardware_version);
          cc->set_firmware_version(firmware_version);
          cc->set_eirp_capability(eirp_capability);
          cc->set_latitude(latitude);
          cc->set_longitude(longitude);
          cc->set_height(height);
          cc->set_height_type(height_type);
          cc->set_horizontal_accuracy(horizontal_accuracy);
          cc->set_vertical_accuracy(vertical_accuracy);
          cc->set_indoor_site(indoor_site);
          cc->set_antenna_azimuth(antenna_azimuth);
          cc->set_antenna_downtilt(antenna_downtilt);
          cc->set_antenna_gain(antenna_gain);
          cc->set_antenna_beamwidth(antenna_beamwidth);
          cc->set_antenna_model(antenna_model);
          cc->set_group_types(gts);
          cc->set_group_ids(gis);
          cc->set_protected_header(protected_header);
          cc->set_encoded_cpi_signed_data(encoded_cpi_signed_data);
          cc->set_digital_signature(digital_signature);
          // optional
          if (c.exists("psi_enabled"))
            cc->set_psi_enabled((bool)c.lookup("psi_enabled"));
          if (c.exists("psi_interval"))
            cc->set_psi_interval((int)c.lookup("psi_interval"));
          if (c.exists("hbt_interval"))
            cc->set_hbt_interval((int)c.lookup("hbt_interval"));
          if (c.exists("channel_blacklist")) {
            list<int> blacklist;
            const settings &bs = c["channel_blacklist"];
            for (const auto &b : bs)
              blacklist.push_back((int)b);
            cc->set_channel_blacklist(blacklist);
          }

          string cert_fn;
          if (c.exists("ap_cert"))
            // override default
            cert_fn = (const char *)c.lookup("ap_cert");
          else
            // default, ap.ROR0010.0CA138FFFFFF.cert.pem
            cert_fn =
                string("ap.") + fcc_id + "." + serial_number + ".cert.pem";
          cc->set_ap_cert(cert_fn);

          string key_fn;
          if (c.exists("ap_key"))
            // override default
            key_fn = (const char *)c.lookup("ap_key");
          else
            // default, ap.ROR0010.0CA138FFFFFF.cert.pem
            key_fn =
                string("ap.") + fcc_id + "." + serial_number + ".key.pem";
          cc->set_ap_key(key_fn);

          if (c.exists("key_passwd"))
            cc->set_key_passwd((const char *)c.lookup("key_passwd"));
          else
            cc->set_key_passwd(string());

          // ostringstream ossc;
          // cc->dump(ossc);
          // cout << ossc.str() << endl;
          sh.insert_config(cc);
        }
      }
    }
    return true;
  } catch (const exception_setting &es) {
    ostringstream oss;
    oss << "exception occurred during parsing settings, " << es.what() << ", "
        << es.getPath();
    m_error = oss.str();
    return false;
  }
}

bool ap_cfg::set_compare(settings &A, settings &B) {
  if(A.getType() != B.getType())
    return false;
  
  switch(A.getType()) {
    case settings::TypeInt: {
      int val_A = A;
      int val_B = B;
      return val_A == val_B;
    }
    case settings::TypeFloat: {
      float val_A = A;
      float val_B = B;
      return val_A == val_B;
    }
    case settings::TypeString: {
      string val_A = A.c_str();
      string val_B = B.c_str();
      return val_A == val_B;
    }
    case settings::TypeBoolean: {
      bool val_A = A;
      bool val_B = B;
      return val_A == val_B;
    }
    case settings::TypeInt64: {
      int64_t val_A = A;
      int64_t val_B = B;
      return val_A == val_B;
    }
    default:
      return false;
  }
}

void print_set_val(settings &s) {
  switch(s.getType()) {
    case settings::TypeInt: {
      int val_s = s;
      std::cout << val_s << endl;
      break;
    }
    case settings::TypeFloat: {
      float val_s = static_cast<float>(s);
      std::cout << val_s << endl;
      break;
    }
    case settings::TypeString: {
      string val_s = s.c_str();
      std::cout << val_s << endl;
      break;
    }
    case settings::TypeBoolean: {
      bool val_s = s;
      std::cout << val_s << endl;
      break;
    }
    case settings::TypeInt64: {
      int64_t val_s = s;
      std::cout << val_s << endl;
      break;
    }
    default:
      std::cout << "Unsupported setting type" << std::endl;
      break;
  }
}

bool ap_cfg::migrate_config(const string &ifn, const string &ofn) {
  ap_cfg cfg_A, cfg_B;
  cfg_A.load_config(ifn.c_str());
  // int in_ver = get_ver();
  cfg_B.load_config(ofn.c_str());
  // int out_ver = get_ver();
  
  try {
    settings &rootA = cfg_A.getRoot();
    settings &rootB = cfg_B.getRoot();

    settings &ds_A = rootA.lookup("sites");
    settings &ds_B = rootB.lookup("sites");
    // site migrate start
    for (int i = 0; i < ds_B.getLength(); ++i) {
      settings &dps_B = ds_B[i];
      settings &dps_A = ds_A[i];
      for (int j = 0; j < dps_B.getLength() - 1; ++j) {
        settings &ds_set_B = dps_B[j];
        // print_set_val(ds_set_B);
        const string ds_b_name = ds_set_B.getName();
        // cout << ds_b_name << endl;
        if (dps_A.exists(ds_b_name)) {
          settings &ds_set_A = dps_A.lookup(ds_b_name);
          // print_set_val(ds_set_A);
          if (!set_compare(ds_set_B, ds_set_A)) {
            modify mod;
            if (ds_b_name == "name") {
              mod.name = ds_b_name + "(dp)";
            } else {
              mod.name = ds_b_name;
            }
            mod.dp = 1;
            mod.val_dif = 1;
            modified.push_back(mod);
            // cout << "mod name: " << mod.name << endl;
          }
        } else {
          added.push_back(ds_b_name);
          // cout << "add name: " << ds_b_name << endl;
        }
      }
      // cout << "modified: " << endl;
      // for (size_t i = 0; i < modified.size(); ++i) {
      //   cout << modified[i].name << endl;
      // }
      // cout << endl << "added: " << endl;
      // for (size_t i = 0; i < added.size(); ++i) {
      //   cout << added[i] << endl;
      // }

      settings &ts_A = dps_A.lookup("buildings");
      settings &ts_B = dps_B.lookup("buildings");
      // building migrate start
      for (int i = 0; i < ts_B.getLength(); ++i) {
        settings &tls_B = ts_B[i];
        settings &tls_A = ts_A[i];
        for (int j = 0; j < tls_B.getLength() - 1; ++j) {
          settings &ts_set_B = tls_B[j];
          const string ts_b_name = ts_set_B.getName();
          if (tls_A.exists(ts_b_name)) {
            settings &ts_set_A = tls_A.lookup(ts_b_name);
            if (!set_compare(ts_set_B, ts_set_A)) {
              modify mod;
              if (ts_b_name == "name" || ts_b_name == "site_name") {
                mod.name = ts_b_name + "(cl)";
              } else {
                mod.name = ts_b_name;
              }
              mod.cl = 1;
              mod.val_dif = 1;
              modified.push_back(mod);
              // cout << "mod name: " << mod.name << endl;
            }
          } else {
            added.push_back(ts_b_name);
            // cout << "add name: " << ts_b_name << endl;
          }
        }
        // cout << "modified: " << endl;
        // for (size_t i = 0; i < modified.size(); ++i) {
        //   cout << modified[i].name << endl;
        // }
        // cout << endl << "added: " << endl;
        // for (size_t i = 0; i < added.size(); ++i) {
        //   cout << added[i] << endl;
        // }

        settings &cs_A = tls_A.lookup("aps");
        settings &cs_B = tls_B.lookup("aps");
        // ap migrate check start
        for (int i = 0; i < cs_B.getLength(); ++i) {
          settings &cbs_B = cs_B[i];
          settings &cbs_A = cs_A[i];
          for (int j = 0; j < cbs_B.getLength(); ++j) {
            settings &cs_set_B = cbs_B[j];
            const string cs_b_name = cs_set_B.getName();
            if (cbs_A.exists(cs_b_name)) {
              settings &cs_set_A = cbs_A.lookup(cs_b_name);
              if (!set_compare(cs_set_B, cs_set_A)) {
                // special case
                if (cs_b_name == "meas_capabilities") {
                  for (int i = 0; i < cs_set_B.getLength(); ++i) {
                    string a = static_cast<const char *>(cs_set_A[i]);
                    string b = static_cast<const char *>(cs_set_B[i]);
                    if (a != b) {
                      modify mod;
                      mod.name = "meas_capabilities";
                      mod.ap = 1;
                      mod.val_dif = 1;
                      modified.push_back(mod);
                      break;
                    }
                  }
                } else if (cs_b_name == "groups") {
                  for (int i = 0; i < cs_set_B.getLength(); ++i) {
                    for (int j = 0; j < cs_set_B[i].getLength(); ++j) {
                      string a = static_cast<const char *>(cs_set_A[i][j]);
                      string b = static_cast<const char *>(cs_set_B[i][j]);
                      // cout << a << endl;
                      // cout << b << endl;
                      // why this will make error??????????????? 
                      // if (static_cast<const char *>(cs_set_A[i][j]) != static_cast<const char *>(cs_set_B[i][j]))
                      if (a != b) {
                        // print_set_val(cs_set_A[i][j]);
                        // print_set_val(cs_set_B[i][j]);
                        modify mod;
                        mod.name = "groups";
                        mod.ap = 1;
                        mod.val_dif = 1;
                        modified.push_back(mod);
                        break;
                      }
                    }
                  }
                } else if (cs_b_name == "channel_blacklist") {
                  for (int i = 0; i < cs_set_B.getLength(); ++i) {
                    if (static_cast<int>(cs_set_A[i]) != static_cast<int>(cs_set_B[i])) {
                      modify mod;
                      mod.name = "channel_blacklist";
                      mod.ap = 1;
                      mod.val_dif = 1;
                      modified.push_back(mod);
                      break;
                    }
                  }
                } else if (cs_b_name == "site_name" || cs_b_name == "building_name"){
                  modify mod;
                  mod.name = cs_b_name + "(cs)";
                  mod.ap = 1;
                  mod.val_dif = 1;
                  modified.push_back(mod);
                } else {
                  modify mod;
                  mod.name = cs_b_name;
                  mod.ap = 1;
                  mod.val_dif = 1;
                  modified.push_back(mod);
                }
              }
            } else {
              added.push_back(cs_b_name);
              // cout << "add name: " << cs_b_name << endl;
            }
          }
          // cout << "modified: " << endl;
          // for (size_t i = 0; i < modified.size(); ++i) {
          //   cout << modified[i].name << endl;
          // }
          // cout << endl << "added: " << endl;
          // for (size_t i = 0; i < added.size(); ++i) {
          //   cout << added[i] << endl;
          // }
        }
        // ap migrate check end
      }
      // building migrate check end
    }
    // site migrate check end

    for (int i = 0; i < ds_A.getLength(); ++i) {
      settings &dps_A = ds_A[i];
      settings &dps_B = ds_B[i];
      for (int j = 0; j < dps_A.getLength() - 1; ++j) {
        settings &ds_set_A = dps_A[j];
        // print_set_val(ds_set_A);
        const string ds_a_name = ds_set_A.getName();
        // cout << ds_b_name << endl;
        if (!dps_B.exists(ds_a_name)) {
          deleted.push_back(ds_a_name);
        }
      }
      cout << "deleted: " << endl;
      for (size_t i = 0; i < deleted.size(); ++i) {
        cout << deleted[i] << endl;
      }

      settings &ts_A = dps_A.lookup("buildings");
      settings &ts_B = dps_B.lookup("buildings");
      for (int i = 0; i < ts_A.getLength(); ++i) {
        settings &tls_A = ts_A[i];
        settings &tls_B = ts_B[i];
        for (int j = 0; j < tls_B.getLength() - 1; ++j) {
          settings &ts_set_A = tls_A[j];
          const string ts_a_name = ts_set_A.getName();
          if (!tls_B.exists(ts_a_name)) {
            deleted.push_back(ts_a_name);
          }
        }
        // cout << "deleted: " << endl;
        // for (size_t i = 0; i < deleted.size(); ++i) {
        //   cout << deleted[i] << endl;
        // } 

        settings &cs_A = tls_A.lookup("aps");
        settings &cs_B = tls_B.lookup("aps");
        // ap migrate check start
        for (int i = 0; i < cs_A.getLength(); ++i) {
          settings &cbs_A = cs_A[i];
          settings &cbs_B = cs_B[i];
          for (int j = 0; j < cbs_A.getLength(); ++j) {
            settings &cs_set_A = cbs_A[j];
            const string cs_a_name = cs_set_A.getName();
            if (cbs_B.exists(cs_a_name)) {
              deleted.push_back(cs_a_name);
            }
          }
          // cout << "deleted: " << endl;
          // for (size_t i = 0; i < deleted.size(); ++i) {
          //   cout << deleted[i] << endl;
          // }
        }
      }
    }
    return true;

    // REMEMBER TO SAVE 
  } catch (const exception_setting &es) {
    ostringstream oss;
    oss << "exception occurred during parsing settings, " << es.what() << ", "
        << es.getPath();
    m_error = oss.str();
    return false;
  }
}

bool ap_cfg::build_config() {
  return true;
}

#if 0
bool ap_cfg::build_config(const string &ofn) {
  shim &sh = shim::instance();
  reset_error();

  try {
    reset();
    readFile(ofn.c_str());
    settings &root = getRoot();
    while (root.getLength() > 0)
      root.remove((unsigned int)0);

    root.add("ver", settings::TypeInt) = m_ver;
    map<uint64_t, string> id_key = sh.get_id_key();
    for (auto ik = id_key.rbegin(); ik != id_key.rend(); ++ik) {
      if (object_config::is_site(ik->first)) {
        site_config_ptr sh_dc =
            dynamic_pointer_cast<site_config>(sh.find_config(ik->second));
        if (!root.exists("sites"))
          root.add("sites", Setting::TypeList);
        settings &ds = root["sites"];
        // sites starts
        settings &ds_1 = ds.add(settings::TypeGroup);
        // mandatory
        ds_1.add("name", settings::TypeString) = sh_dc->get_name();
        // optional
        ds_1.add("tls_dns_cache", settings::TypeString) = sh_dc->get_dns_cache();
        ds_1.add("tls_dns_interval", settings::TypeInt64) = sh_dc->get_dns_interval();
        ds_1.add("tls_local_port", settings::TypeInt) = sh_dc->get_local_port();
        ds_1.add("tls_maxage_conn", settings::TypeInt64) = sh_dc->get_maxage_conn();
        ds_1.add("tls_conn_timeout", settings::TypeInt64) = sh_dc->get_conn_timeout();
        ds_1.add("tls_ip_resolve", settings::TypeInt64) = sh_dc->get_ip_resolve();
        ds_1.add("tls_interface", settings::TypeString) = sh_dc->get_interface();
        ds_1.add("tls_tcp_keepalive_idle", settings::TypeInt64) = sh_dc->get_tcp_keepalive_idle();
        ds_1.add("tls_tcp_keepalive_interval", settings::TypeInt64) = sh_dc->get_tcp_keepalive_interval();
        ds_1.add("tls_tls_debug", settings::TypeBoolean) = sh_dc->get_tls_debug();
      }

      if (object_config::is_building(ik->first)) {
        building_config_ptr sh_tc =
            dynamic_pointer_cast<building_config>(sh.find_config(ik->second));
        settings &ds = root["sites"];
        // get site name that current building belongs to
        string tc_dc_name = sh_tc->get_site_name();
        int count_dc = 0;
        for (auto &dps : ds) {
          const char *name = static_cast<const char *>(dps.lookup("name"));
          if (name == tc_dc_name) {
            break;
          }
          count_dc++;
        }
        settings &ds_1 = ds[count_dc];
        if (!ds_1.exists("buildings"))
          ds_1.add("buildings", Setting::TypeList);
        settings &ts = ds_1["buildings"];

        // building starts
        settings &ts_1 = ts.add(settings::TypeGroup);
        // mandatory
        ts_1.add("name", settings::TypeString) = sh_tc->get_name();
        ts_1.add("site_name", settings::TypeString) = sh_tc->get_site_name();
        ts_1.add("sas_url", settings::TypeString) = sh_tc->get_sas_url();
        ts_1.add("user_id", settings::TypeString) = sh_tc->get_user_id();
        ts_1.add("ca_path", settings::TypeString) = sh_tc->get_ca_path();
        // optional
        ts_1.add("root_ca", settings::TypeString) = sh_tc->get_root_ca();
        ts_1.add("sas_crl", settings::TypeString) = sh_tc->get_sas_crl();
      }

      if (object_config::is_ap(ik->first)) {
        ap_config_ptr sh_cc =
            dynamic_pointer_cast<ap_config>(sh.find_config(ik->second));
        settings &ds = root["sites"];
        // get site name that current ap belongs to
        string cc_dc_name = sh_cc->get_site_name();
        string cc_tc_name = sh_cc->get_building_name();
        int count_dc = 0;
        int count_tc = 0;
        for (auto &dps : ds) {
          const char *name = static_cast<const char *>(dps.lookup("name"));
          if (name == cc_dc_name) {
            break;
          }
          count_dc++;
        }
        settings &ds_1 = ds[count_dc];

        // get building name that current ap belongs to
        settings &ts = ds_1["buildings"];
        for (auto &cls : ts) {
          const char *name = static_cast<const char *>(cls.lookup("name"));
          if (name == cc_tc_name) {
            break;
          }
          count_tc++;
        }
        settings &ts_1 = ts[count_tc];
        if (!ts_1.exists("aps"))
          ts_1.add("aps", Setting::TypeList);
        settings &cs = ts_1["aps"];

        // aps starts
        settings &cs_1 = cs.add(settings::TypeGroup);
        // mandatory
        cs_1.add("site_name", settings::TypeString) = sh_cc->get_site_name();
        cs_1.add("building_name", settings::TypeString) = sh_cc->get_building_name();
        cs_1.add("admin_state", settings::TypeBoolean) = sh_cc->get_admin_state();
        cs_1.add("single_step", settings::TypeBoolean) = sh_cc->get_single_step();
        cs_1.add("central_freq_khz", settings::TypeInt) = static_cast<int>(sh_cc->get_central_freq_khz());
        cs_1.add("radio_bandwidth_mhz", settings::TypeInt) = static_cast<int>(sh_cc->get_radio_bandwidth_mhz());
        cs_1.add("fcc_id", settings::TypeString) = sh_cc->get_fcc_id();
        cs_1.add("serial_number", settings::TypeString) = sh_cc->get_serial_number();
        cs_1.add("cs_1_name", settings::TypeString) = sh_cc->get_name();
        cs_1.add("category", settings::TypeString) = sh_cc->get_category();
        cs_1.add("call_sign", settings::TypeString) = sh_cc->get_call_sign();
        cs_1.add("meas_capabilities", settings::TypeArray);
        settings &mcs = cs_1["meas_capabilities"];
        list<string> meas_caps;
        meas_caps = sh_cc->get_meas_capabilities();
        for (const auto &m : meas_caps) {
          mcs.add(settings::TypeString) = m;
        }
        cs_1.add("radio_technology", settings::TypeString) = sh_cc->get_radio_technology();
        cs_1.add("vendor", settings::TypeString) = sh_cc->get_vendor();
        cs_1.add("model", settings::TypeString) = sh_cc->get_model();
        cs_1.add("software_version", settings::TypeString) = sh_cc->get_software_version();
        cs_1.add("hardware_version", settings::TypeString) = sh_cc->get_hardware_version();
        cs_1.add("firmware_version", settings::TypeString) = sh_cc->get_firmware_version();
        cs_1.add("eirp_capability", settings::TypeInt) = sh_cc->get_eirp_capability();
        cs_1.add("latitude", settings::TypeFloat) = sh_cc->get_latitude();
        cs_1.add("longitude", settings::TypeFloat) = sh_cc->get_longitude();
        cs_1.add("height", settings::TypeFloat) = sh_cc->get_height();
        cs_1.add("height_type", settings::TypeString) = sh_cc->get_height_type();
        cs_1.add("horizontal_accuracy", settings::TypeFloat) = sh_cc->get_horizontal_accuracy();
        cs_1.add("vertical_accuracy", settings::TypeFloat) = sh_cc->get_vertical_accuracy();
        cs_1.add("indoor_site", settings::TypeBoolean) = sh_cc->get_indoor_site();
        cs_1.add("antenna_azimuth", settings::TypeInt) = sh_cc->get_antenna_azimuth();
        cs_1.add("antenna_downtilt", settings::TypeInt) = sh_cc->get_antenna_downtilt();
        cs_1.add("antenna_gain", settings::TypeInt) = sh_cc->get_antenna_gain();
        cs_1.add("antenna_beamwidth", settings::TypeInt) = sh_cc->get_antenna_beamwidth();
        cs_1.add("antenna_model", settings::TypeString) = sh_cc->get_antenna_model();
        cs_1.add("groups", settings::TypeList);
        settings &groups = cs_1["groups"];
        list<string> gts, gis;
        gts = sh_cc->get_group_types();
        gis = sh_cc->get_group_ids();
        int group_length = gts.size();
        for (int i = 0; i < group_length; i++) {
          settings &gp = groups.add(settings::TypeGroup);
          list<string>::iterator it_type, it_id;
          it_type = gts.begin();
          it_id = gis.begin();
          advance(it_type, i);
          advance(it_id, i);
          string type, id;
          type = *it_type;
          id = *it_id;
          gp.add("type", settings::TypeString) = type;
          gp.add("id", settings::TypeString) = id;
        }
        cs_1.add("protected_header", settings::TypeString) = sh_cc->get_protected_header();
        cs_1.add("encoded_cpi_signed_data", settings::TypeString) = sh_cc->get_encoded_cpi_signed_data();
        cs_1.add("digital_signature", settings::TypeString) = sh_cc->get_digital_signature();
        // optional
        cs_1.add("psi_enabled", settings::TypeBoolean) = sh_cc->get_psi_enabled();
        cs_1.add("psi_interval", settings::TypeInt) = sh_cc->get_psi_interval();
        cs_1.add("hbt_interval", settings::TypeInt) = sh_cc->get_hbt_interval();
        cs_1.add("channel_blacklist", settings::TypeList);
        settings &bs = cs_1["channel_blacklist"];
        list<int> blacklist = sh_cc->get_channel_blacklist();
        for (const auto &b : blacklist) {
          bs.add(settings::TypeInt) = b;
        }
        cs_1.add("ap_cert", settings::TypeString) = sh_cc->get_ap_cert();
        cs_1.add("ap_key", settings::TypeString) = sh_cc->get_ap_key();
        cs_1.add("key_passwd", settings::TypeString) = sh_cc->get_key_passwd();
      }
    }
    return true;
  }
  catch (const exception_setting &es) {
    ostringstream oss;
    oss << "exception occurred during parsing settings, " << es.what() << ", "
        << es.getPath();
    m_error = oss.str();
    return false;
  }
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// shim_cfg
// class of loading/saving libconfig file based on shim config object meta info
//
///////////////////////////////////////////////////////////////////////////////

string shim_cfg::get_parent_path(const settings &node) {
  string path = node.getPath();
  size_t pos;
  if ((pos = path.find("aps")) != string::npos) 
    return path.substr(0, pos - 1);
  else if ((pos = path.find("buildings")) != string::npos)
    return path.substr(0, pos - 1);
  else
    return getRoot().getPath();
}

void shim_cfg::traverse(const settings &node) {
  string node_name = node.getName();
  for (const auto &n : node) {
    // string noden_name = n.getName();
    // cout << "1: " << noden_name;
    object_config_ptr oc = nullptr;
    meta_map *mm = nullptr;
    member_map *bm = nullptr;
    if (node_name == "sites") {
      auto dc = site_config::create();
      mm = &(dc->get_meta());
      bm = &(dc->get_members());
      oc = dc;
    }
    else if (node_name == "buildings") {
      auto tc = building_config::create();
      mm = &(tc->get_meta());
      bm = &(tc->get_members());
      oc = tc;
    } 
    else if (node_name == "aps") {
      auto cc = ap_config::create();
      mm = &(cc->get_meta());
      bm = &(cc->get_members());
      oc = cc;
    }
    else
      throw runtime_error(string("unknown node, ") + node.getName());
    // set version
    oc->set_ver(m_ver);

    for (auto &v : *mm) {
      const string &var = v.first;
      const meta_t &t = v.second;
      const member_t &b = (*bm)[var];
      if (t.m_type == "string") {
        if (n.exists(t.m_node)) {
          string val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "bool") {
        if (n.exists(t.m_node)) {
          bool val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "int") {
        if (n.exists(t.m_node)) {
          int val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "int32_t") {
        if (n.exists(t.m_node)) {
          int32_t val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "long") {
        if (n.exists(t.m_node)) {
          long val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "unsigned") {
        if (n.exists(t.m_node)) {
          unsigned val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "double") {
        if (n.exists(t.m_node)) {
          double val = n.lookup(t.m_node);
          b.m_xetter->set(b.m_var, val);
        }
        else
          if (t.m_trait == enum_var_required)
            throw runtime_error(string(var) + "is required");
      }
      else if (t.m_type == "list<int>") {
        size_t pos = t.m_node.find("[");
        if (pos != string::npos) {
          // handle the non-leaf list
          list<int> vals;
          const settings &sn = n.lookup(t.m_node.substr(0, pos));
          int len = sn.getLength();
          for (int i = 0; i < len; ++i)
          {
              char path[BUFSIZ] = { 0 };
              snprintf(path, sizeof(path), t.m_node.c_str(), i);
              vals.push_back((int)n.lookup(path));
          }
          b.m_xetter->set(b.m_var, vals);
        }
        else {
          // handle the leaf list, channel-blacklist
          if (n.exists(t.m_node)) {
            list<int> vals;
            const settings &sn = n.lookup(t.m_node);
            for (const auto &e : sn)
              vals.push_back((int)e);
            b.m_xetter->set(b.m_var, vals);
          }
          else
            if (t.m_trait == enum_var_required)
              throw runtime_error(string(var) + " is required");
        }
      }
      else if (t.m_type == "list<string>") {
        size_t pos = t.m_node.find("[");
        if (pos != string::npos) {
          // handle the non-leaf list
          list<string> vals;
          const settings &sn = n.lookup(t.m_node.substr(0, pos));
          int len = sn.getLength();
          for (int i = 0; i < len; ++i)
          {
              char path[BUFSIZ] = { 0 };
              snprintf(path, sizeof(path), t.m_node.c_str(), i);
              vals.push_back((string)n.lookup(path));
          }
          b.m_xetter->set(b.m_var, vals);
        }
        else {
          // handle the leaf list, channel-blacklist
          if (n.exists(t.m_node)) {
            list<string> vals;
            const settings &sn = n.lookup(t.m_node);
            for (const auto &e : sn)
              vals.push_back((string)e);
            b.m_xetter->set(b.m_var, vals);
          }
          else
            if (t.m_trait == enum_var_required)
              throw runtime_error(string(var) + " is required");
        }
      }
      else
        cerr << "unknown type, " << t.m_type << "; title, " << t.m_node << endl;
    }
    // TODO:
    // 1. iterate with less hardcode
    // 2. handle composed fields in more elegant way
    if (node_name == "sites") {
      shim::instance().insert_config(oc);
      traverse(n["buildings"]);
    }
    else if (node_name == "buildings") {
            
      // handle composed field of name
      string site = get_parent_path(n);
      building_config_ptr tc = std::dynamic_pointer_cast<building_config>(oc);
      tc->set_site_name((string)getRoot().lookup(site + ".name"));
      shim::instance().insert_config(oc);
      traverse(n["aps"]);
    }
    else if (node_name == "aps") {
      // handle composed fields of parent names and ap name
      string building = get_parent_path(n);  
      settings &c = getRoot().lookup(building);
      string site = get_parent_path(c);
      ap_config_ptr cc = std::dynamic_pointer_cast<ap_config>(oc);
      cc->set_site_name((string)getRoot().lookup(site + ".name"));
      cc->set_building_name((string)getRoot().lookup(building + ".name"));
      cc->set_name(cc->get_fcc_id() + ":" + cc->get_serial_number());
      shim::instance().insert_config(oc);
    }
  }
  
}

bool shim_cfg::parse_config() {
  try {
    cout << "ver = " << m_ver << endl;

    // shim layer instance
    shim &sh = shim::instance();
    (void)sh;
    reset_error();
    traverse(getRoot()["sites"]);

    return true;
  } catch (const exception_setting &es) {
    ostringstream oss;
    oss << "exception occurred during parsing settings, " << es.what()
        << ", " << es.getPath();
    m_error = oss.str();
    return false;
  }
}

object_config_ptr shim_cfg::duplicate(const object_config_ptr &s, int dst_ver) {
  object_config_ptr d = nullptr;
  try {
      // reset meta map ptr
      m_src_meta = nullptr;
      m_dst_meta = nullptr;
      int src_ver = s->get_ver();
      if (object_config::is_site(s->get_map_id())) {
        // source
        switch (src_ver) {
          case enum_ver_1: {
            m_src_meta = &(site_config_v1::get_meta());
            break;
          }
          case enum_ver_2: {
            m_src_meta = &(site_config_v2::get_meta());
            break;
          }
          default:
            throw runtime_error(string("unsupported source version, ") + to_string(src_ver));
            break;
          }
        // target
        switch (dst_ver) {
          case enum_ver_1: {
            d = site_config_v1::create();
            if (d) {
                m_dst_meta = &(site_config_v1::get_meta());
                m_dst_objs.push_back(d);
            }
            break;
          }
          case enum_ver_2: {
            d = site_config_v2::create();
            if (d) {
                m_dst_meta = &(site_config_v2::get_meta());
                m_dst_objs.push_back(d);
            }
            break;
          }
          default:
            throw runtime_error(string("unsupported target version, ") + to_string(dst_ver));
        }
      }
      else if (object_config::is_building(s->get_map_id())) {
         // source
        switch (src_ver) {
          case enum_ver_1: {
            m_src_meta = &(building_config_v1::get_meta());
            break;
          }
          case enum_ver_2: {
            m_src_meta = &(building_config_v2::get_meta());
            break;
          }
          default:
            throw runtime_error(string("unsupported source version, ") + to_string(src_ver));
        }
        // target
        switch (dst_ver) {
          case enum_ver_1: {
            d = building_config_v1::create();
            if (d) {
                m_dst_meta = &(building_config_v1::get_meta());
                m_dst_objs.push_back(d);
            }
            break;
          }
          case enum_ver_2: {
            d = building_config_v2::create();
            if (d) {
                m_dst_meta = &(building_config_v2::get_meta());
                m_dst_objs.push_back(d);
            }
            break;
          }
          default:
            throw runtime_error(string("unsupported target version, ") + to_string(dst_ver));
        }
      }
      else if (object_config::is_ap(s->get_map_id())) {
        // source
        switch (src_ver) {
          case enum_ver_1: {
              m_src_meta = &(ap_config_v1::get_meta());
              break;
          }
          case enum_ver_2: {
              m_src_meta = &(ap_config_v2::get_meta());
              break;
          }
          default:
              throw runtime_error(string("unsupported source version, ") + to_string(src_ver));
        }
        // target
        switch (dst_ver) {
          case enum_ver_1: {
              d = ap_config_v1::create();
              if (d) {
                  m_dst_meta = &(ap_config_v1::get_meta());
                  m_dst_objs.push_back(d);
              }
              break;
          }
          case enum_ver_2: {
              d = ap_config_v2::create();
              if (d) {
                  m_dst_meta = &(ap_config_v2::get_meta());
                  m_dst_objs.push_back(d);
              }
              break;
          }
          default:
              throw runtime_error(string("unsupported target version, ") + to_string(dst_ver));
        }
      }
      else 
        throw runtime_error(string("unknown config type") + to_string(s->get_map_id()));

      // set version
      d->set_ver(dst_ver);
      return d;
  }
  catch (const exception &e) {
      cerr << e.what() << endl;
      return nullptr;
  }
}

bool shim_cfg::migrate_config(int dst_ver) {
  m_ver = dst_ver;

  try {
    shim &sh = shim::instance();
    vector<object_config_ptr> ordered_oc = sh.get_ordered_oc();
    sh.clear_ordered_oc();
    for (const auto &s : ordered_oc) {
      // step 1: duplicate
      object_config_ptr d = duplicate(s, dst_ver);
      if (!d)
        throw runtime_error(string("failed to duplicate object config, ") + s->get_key());

      // step 2: diff meta info
      const meta_map &src_meta = *m_src_meta;
      const meta_map &dst_meta = *m_dst_meta;

      list<string> added, deleted, changed, unchanged;
      for (const auto &sp : src_meta) {
        if (dst_meta.find(sp.first) == dst_meta.end()) {
          deleted.push_back(sp.first);
        }
        else {
          if (sp.second == dst_meta.at(sp.first)) {
            unchanged.push_back(sp.first);
          }
          else 
            changed.push_back(sp.first);
        }
      }
      for (const auto &dp : dst_meta) {
        if (src_meta.find(dp.first) == src_meta.end())
          added.push_back(dp.first);
      }
      set_added(added);

#if 0
      // dump
      // +++++ added
      // ----- deleted
      // ***** changed
      // ===== unchanged
      cout << "##### config, " << s->get_key() << endl;
      for (auto va : added)
          cout << "+++++ var = " << va << endl;
      for (auto vd : deleted)
          cout << "----- var = " << vd << endl;
      for (auto vc : changed)
          cout << "***** var = " << vc << endl;
      for (auto vu : unchanged)
          cout << "===== var = " << vu << endl;
#endif

      // step 3: init target config

      // in compile time, use the macro, based on the type, create get and set corresponding with different settings
      // in run time, create new object and call init, bring up the meta data that already builded in compile time
      for (auto vu : unchanged) {
        std::any v;
        if (s->get(vu, v))
          d->set(vu, v);
      }
      for (auto vc : changed) {
        std::any v;
        if (s->get(vc, v))
          d->set(vc, v);
      }

      // step 4: insert new config to shim store and remove original
      sh.delete_config(s->get_map_id());
      sh.insert_config(d);
#if 0
      // dump new version config
      // d->dump();
      d->dump_meta();
#endif

    }

    return true;
  }
  catch (const exception &e)
  {
    cerr << e.what() << endl;
    return false;
  }
}

void shim_cfg::build_traverse(shim &sh) {
  try {
    reset_result_cfg();
    settings &root = result_cfg.getRoot();
    root.add("ver", settings::TypeInt) = m_ver;
    vector<object_config_ptr> ordered_oc = sh.get_ordered_oc();
    for (auto &o : ordered_oc) {
      member_map *bm = &(o->get_members());
      meta_map *mm = &(o->get_meta_info());
      settings *op = nullptr;

      if(object_config::is_site(o->get_map_id())){
        if (!root.exists("sites"))
          root.add("sites", settings::TypeList);
        settings &dc = root.lookup("sites");
        settings &ds = dc.add(settings::TypeGroup);
        op = &ds;
      }
      else if (object_config::is_building(o->get_map_id())) {
        std::any v;
        (*bm)["site_name"].m_xetter->get((*bm)["site_name"].m_var, v);
        string dc_name = any_cast<string>(v);
        settings &dc = root["sites"];
        int count_dc = 0;
        for (auto &d : dc) {
          const char *name = static_cast<const char *>(d.lookup("name"));
          if (name == dc_name)
            break;
          count_dc++;
        }
        settings &ds = dc[count_dc];
        if (!ds.exists("buildings"))
          ds.add("buildings", Setting::TypeList);
        settings &tc = ds["buildings"];
        settings &ts = tc.add(settings::TypeGroup);
        op = &ts;
      }
      else if (object_config::is_ap(o->get_map_id())) {
        std::any v_1, v_2;
        (*bm)["site_name"].m_xetter->get((*bm)["site_name"].m_var, v_1);
        (*bm)["building_name"].m_xetter->get((*bm)["building_name"].m_var, v_2);
        string dc_name = any_cast<string>(v_1);
        string tc_name = any_cast<string>(v_2);
        cout << dc_name << " " << tc_name << endl;
        settings &dc = root["sites"];
        int count_ds = 0;
        int count_ts = 0;
        for (auto &d : dc) {
          const char *name = static_cast<const char *>(d.lookup("name"));
          if (name == dc_name)
            break;
          count_ds++;
        }
        cout << count_ds << endl;
        settings &ds = dc[count_ds];
        settings &tc = ds["buildings"];
        for (auto &t : tc) {
          const char *name = static_cast<const char *>(t.lookup("name"));
          if (name == tc_name)
            break;
          count_ts++;
        }
        settings &ts = tc[count_ts];
        if (!ts.exists("aps"))
          ts.add("aps", Setting::TypeList);
        settings &cc = ts["aps"];
        settings &cs = cc.add(settings::TypeGroup);
        op = &cs;
      }
      

      for (auto &b : *bm) {
        std::any v;
        b.second.m_xetter->get(b.second.m_var, v);
        meta_t m = (*mm)[b.first];
        if (m.m_type == "int" || m.m_type == "int32_t") {
          if (v.type() == typeid(int)) {
            int val = any_cast<int>(v);
            (*op).add(b.first, settings::TypeInt) = val;
          }
          else
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "unsigned") {
          if (v.type() == typeid(unsigned)) {
            unsigned int val = any_cast<unsigned int>(v);
            (*op).add(b.first, settings::TypeInt) = static_cast<int>(val);
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "long") {
          if (v.type() == typeid(long)) {
            long val = any_cast<long>(v);
            (*op).add(b.first, settings::TypeInt64) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "float") {
          if (v.type() == typeid(float)) {
            float val = any_cast<float>(v);
            (*op).add(b.first, settings::TypeFloat) = val;
          }
          else
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "double") {
          if (v.type() == typeid(double)) {
            double val = any_cast<double>(v);
            (*op).add(b.first, settings::TypeFloat) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "string") {
          if (v.type() == typeid(string)) {
            string val = any_cast<string>(v);
            (*op).add(b.first, settings::TypeString) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "bool") {
          if (v.type() == typeid(bool)) {
            bool val = any_cast<bool>(v);
            (*op).add(b.first, settings::TypeBoolean) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "list<int>") {
          if (v.type() == typeid(list<int>)) {
            list<int> val = any_cast<list<int>>(v);
            settings &temp = (*op).add(b.first, settings::TypeArray);
            for (auto i : val) {
              temp.add(settings::TypeInt) = i;
            }
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "list<string>") {
          if (v.type() == typeid(list<string>)) {
            list<string> val = any_cast<list<string>>(v);
            settings &temp = (*op).add(b.first, settings::TypeArray);
            for (auto i : val) {
              temp.add(settings::TypeString) = i;
            }
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else {
          list<string> added = get_added();
          for (auto &a : added) {
            meta_t ad = (*mm)[a];
            string type = ad.m_type;
            if (type == "int" || type == "int32_t")
              (*op).add(a, settings::TypeInt);
            else if (type == "long")
              (*op).add(a, settings::TypeInt64);
            else if (type == "float" || type == "double")
              (*op).add(a, settings::TypeFloat);
            else if (type == "string")
              (*op).add(a, settings::TypeString);
            else if (type == "bool")
              (*op).add(a, settings::TypeBoolean);
            else if (type == "list<int>" || type == "list<string>")
              (*op).add(a, settings::TypeArray);
          }
        }
      }
    }
  }
  catch (const exception &e) {
    cerr << "fail to build" << e.what() << endl;
  }
}

bool shim_cfg::build_config() {
  try {
    shim &sh = shim::instance();
    reset();

    build_traverse(sh);

    return true;
  }
  catch (const exception &e) {
    cerr << "fail to build" << e.what() << endl;
    return false;
  }
}



#if 0
bool shim_cfg::build_config() {
  shim &sh = shim::instance();
  reset();

  try {
    settings &root = result_cfg.getRoot();
    root.add("ver", settings::TypeInt) = m_ver;

    m_src_objs = sh.find_all_config();
    map<uint64_t, string> id_key = sh.get_id_key();
    
    for (auto &o : sh.get_ordered_oc()) {
      member_map *bm = nullptr;
      meta_map *mm = nullptr;
      settings *op = nullptr;
      if(object_config::is_site(o->get_map_id())){
        site_config_ptr sh_dc = dynamic_pointer_cast<site_config>(o);
        bm = &(sh_dc->get_members());
        mm = &(sh_dc->get_meta());
        if (!root.exists("sites"))
          root.add("sites", settings::TypeList);
        settings &dc = root.lookup("sites");
        settings &ds = dc.add(settings::TypeGroup);
        op = &ds;
      }
      else if (object_config::is_building(o->get_map_id())) {
        building_config_ptr sh_tc = dynamic_pointer_cast<building_config>(o);
        bm = &(sh_tc->get_members());
        mm = &(sh_tc->get_meta());

        string dc_name = sh_tc->get_site_name();
        settings &dc = root["sites"];
        int count_dc = 0;
        for (auto &d : dc) {
          const char *name = static_cast<const char *>(d.lookup("name"));
          if (name == dc_name) {
            break;
          }
          count_dc++;
        }
        settings &ds = dc[count_dc];
        if (!ds.exists("buildings"))
          ds.add("buildings", Setting::TypeList);
        settings &tc = ds["buildings"];
        settings &ts = tc.add(settings::TypeGroup);
        op = &ts;
      }
      else if (object_config::is_ap(o->get_map_id())) {
        ap_config_ptr sh_cc = dynamic_pointer_cast<ap_config>(o);
        bm = &(sh_cc->get_members());
        mm = &(sh_cc->get_meta());
        string dc_name = sh_cc->get_site_name();
        string tc_name = sh_cc->get_building_name();
        settings &dc = root["sites"];
        int count_ds = 0;
        int count_ts = 0;
        for (auto &d : dc) {
          const char *name = static_cast<const char *>(d.lookup("name"));
          if (name == dc_name) {
            break;
          }
          count_ds++;
        }
        settings &ds = dc[count_ds];
        settings &tc = ds["buildings"];
        for (auto &t : tc) {
          const char *name = static_cast<const char *>(t.lookup("name"));
          if (name == tc_name) {
            break;
          }
          count_ts++;
        }
        settings &ts = tc[count_ts];
        if (!ts.exists("aps"))
          ts.add("aps", Setting::TypeList);
        settings &cc = ts["aps"];
        settings &cs = cc.add(settings::TypeGroup);
        op = &cs;
      }

      for (auto &b : *bm) {
        cout << b.first << endl;
        std::any v;
        b.second.m_xetter->get(b.second.m_var, v);
        // cout << v.type().name() << endl;
        meta_t m = (*mm)[b.first];
        // cout << b.first << " " << m.m_type << endl;
        // cout << "ok" << endl;
        if (m.m_type == "int" || m.m_type == "int32_t") {
          if (v.type() == typeid(int)) {
            int val = any_cast<int>(v);
            (*op).add(b.first, settings::TypeInt) = val;
          }
          else
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "unsigned") {
          if (v.type() == typeid(unsigned)) {
            unsigned int val = any_cast<unsigned int>(v);
            (*op).add(b.first, settings::TypeInt) = static_cast<int>(val);
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "long") {
          if (v.type() == typeid(long)) {
            long val = any_cast<long>(v);
            (*op).add(b.first, settings::TypeInt64) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "float") {
          if (v.type() == typeid(float)) {
            float val = any_cast<float>(v);
            (*op).add(b.first, settings::TypeFloat) = val;
          }
          else
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "double") {
          if (v.type() == typeid(double)) {
            double val = any_cast<double>(v);
            (*op).add(b.first, settings::TypeFloat) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "string") {
          if (v.type() == typeid(string)) {
            string val = any_cast<string>(v);
            (*op).add(b.first, settings::TypeString) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "bool") {
          if (v.type() == typeid(bool)) {
            bool val = any_cast<bool>(v);
            (*op).add(b.first, settings::TypeBoolean) = val;
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "list<int>") {
          if (v.type() == typeid(list<int>)) {
            list<int> val = any_cast<list<int>>(v);
            settings &temp = (*op).add(b.first, settings::TypeArray);
            for (auto i : val) {
              temp.add(settings::TypeInt) = i;
            }
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
        else if (m.m_type == "list<string>") {
          if (v.type() == typeid(list<string>)) {
            list<string> val = any_cast<list<string>>(v);
            settings &temp = (*op).add(b.first, settings::TypeArray);
            for (auto i : val) {
              temp.add(settings::TypeString) = i;
            }
          }
          else 
            throw runtime_error("unexpected type" + b.first);
        }
      }
    }

    return true;
  }
  catch (const exception &e) {
    cerr << "fail to build" << e.what() << endl;
    return false;
  }
}
#endif

} // namespace project




