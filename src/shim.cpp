#include <cstdint>
#include <iomanip>
#include <memory>

#include "config.h"
#include "shim.h"

using namespace project;
using namespace std;

namespace project {

///////////////////////////////////////////////////////////////////////////////
//
// object_config
// base class of object configuration
//
///////////////////////////////////////////////////////////////////////////////

void object_config::dump(ostream &os /* = std::cout */) {
    if (is_site(m_map_id))
        os << "site_config#" << m_obj_id;
    else if (is_building(m_map_id))
        os << "building_config#" << m_obj_id;
    else if (is_ap(m_map_id))
        os << "ap_config#" << m_obj_id;

    os << ", " << get_key() << endl
        << "  ver = " << get_ver() << endl;

    for (auto b : m_members)
    {
        // get var value via xetter
        std::any v;
        const member_t &m = b.second;
        m.m_xetter->get(m.m_var, v);
        // output var
        os << "  " << b.first
            << " = ";
        if (v.type() == typeid(int8_t))
            os << (int)std::any_cast<int8_t>(v);
        else if (v.type() == typeid(int16_t))
            os << std::any_cast<int16_t>(v);
        else if (v.type() == typeid(int32_t))
            os << std::any_cast<int32_t>(v);
        else if (v.type() == typeid(int64_t))
            os << std::any_cast<int64_t>(v);
        else if (v.type() == typeid(uint8_t))
            os << (int)std::any_cast<uint8_t>(v);
        else if (v.type() == typeid(uint16_t))
            os << std::any_cast<uint16_t>(v);
        else if (v.type() == typeid(uint32_t))
            os << std::any_cast<uint32_t>(v);
        else if (v.type() == typeid(uint64_t))
            os << std::any_cast<uint64_t>(v);
        else if (v.type() == typeid(string))
            os << std::any_cast<string>(v);
        else if (v.type() == typeid(char *))
            os << std::any_cast<char *>(v);
        else if (v.type() == typeid(bool))
            os << boolalpha << std::any_cast<bool>(v);
        else if (v.type() == typeid(double))
            os << std::any_cast<double>(v);
        else if (v.type() == typeid(list<int>))
        {
            os << "[" << endl
                << "    ";
            const list<int> &vs = std::any_cast<list<int>>(v);
            for (list<int>::const_iterator cit = vs.cbegin();
                    cit != vs.cend(); ++cit)
                os << (cit == vs.cbegin() ? "" : ", ") << *cit;
            os << (vs.empty() ? "(empty)" : "") << endl
                << "  ]";
        }
        else if (v.type() == typeid(list<string>))
        {
            os << "[" << endl
                << "    ";
            const list<string> &vs = std::any_cast<list<string>>(v);
            for (list<string>::const_iterator cit = vs.cbegin();
                    cit != vs.cend(); ++cit)
                os << (cit == vs.cbegin() ? "" : ", ") << *cit;
            os << (vs.empty() ? "(empty)" : "") << endl
                << "  ]";
        }
        else
            os << "(un-handled type)";
        os << endl;
    }
}

void object_config::dump_meta(ostream &os /* = std::cout */) {
    os << "meta info:" << endl;
    for (auto v : get_meta_info())
      os << "  " << v.first
          << ", " << v.second.to_string()
          << endl;
}

object_config_ptr object_config::create_site_config(int ver) {
  if (ver == enum_ver_1) 
    return site_config_v1::create();
  else
    return nullptr;
}

object_config_ptr object_config::create_building_config(int ver) {
    if (ver == enum_ver_1)
        return building_config_v1::create();
    else
        return nullptr;
}

object_config_ptr object_config::create_ap_config(int ver) {
    if (ver == enum_ver_1)
        return ap_config_v1::create();
    if (ver == enum_ver_2)
        return ap_config_v2::create();
    else
        return nullptr;
}

bool object_config::set(const string &var, const std::any &val) {
  try {
    if (m_members.find(var) != m_members.end()) {
      member_t &b = m_members[var];
      b.m_xetter->set(b.m_var, val);
      return true;
    }
    else 
      return false;
  } catch (const exception &e) {
    cerr << e.what() << endl;
      return false;
  }
}

bool object_config::get(const string &var, std::any &val) {
  try {
    if (m_members.find(var) != m_members.end()) {
      member_t &b = m_members[var];
      b.m_xetter->get(b.m_var, val);
      return true;
    }
    else 
      return false;
  } catch (const exception &e) {
    cerr << e.what() << endl;
      return false;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// site_config
// struct of site configuration
//
///////////////////////////////////////////////////////////////////////////////

int site_config::s_n_site = 0;

site_config::site_config() {
  m_obj_id = s_n_site++;
  generate_map_id();

  // initialize optional
  m_ap_mode = 0;
  m_dns_interval = 86400;
  m_local_port = 0;
  m_maxage_conn = 118;
  m_conn_timeout = 300;
  m_ip_resolve = 0;
  m_tcp_keepalive_idle = 60;
  m_tcp_keepalive_interval = 60;
  m_tls_debug = false;
}

#if 0
void site_config::dump(ostream &os /* = std::cout */) {
  os << "site_config#" << hex << m_obj_id << dec << ", " << m_name << endl
     << "  ap_mode = " << ap_modes[m_ap_mode] << endl
     << "  spectrum_controller_host = " << m_spectrum_controller_host << endl
     << "  dns_cache = " << m_dns_cache << endl
     << "  dns_interval = " << m_dns_interval << endl
     << "  local_port = " << m_local_port << endl
     << "  maxage_conn = " << m_maxage_conn << endl
     << "  conn_timeout = " << m_conn_timeout << endl
     << "  ip_resolve = " << m_ip_resolve << endl
     << "  interface = " << m_interface << endl
     << "  tcp_keepalive_idle = " << m_tcp_keepalive_idle << endl
     << "  tcp_keepalive_inteval = " << m_tcp_keepalive_interval << endl
     << "  tls_debug = " << boolalpha << m_tls_debug << endl
     << endl
     << "meta info:" << endl;
  
  for (auto v : get_meta())
        os << "  " << v.first
            << ", " << v.second.to_string()
            << endl;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// building_config
// struct of building configuration
//
///////////////////////////////////////////////////////////////////////////////

int building_config::s_n_building = 0;

building_config::building_config() {
  m_obj_id = s_n_building++;
  generate_map_id();
  // initialize optional
  m_root_ca = "/";
  m_sas_crl = "/test.crl";
}

#if 0
void building_config::dump(ostream &os /* = std::cout */) {
  os << "building_config#" << hex << m_obj_id << dec << ", " << m_name << endl
     << "  sas_url = " << m_sas_url << endl
     << "  user_id = " << m_user_id << endl
     << "  ca_path = " << m_ca_path << endl
     << "  root_ca = " << m_root_ca << endl
     << "  sas_crl = " << m_sas_crl << endl
     << endl
     << "meta info:" << endl;

  for (auto v : get_meta())
        os << "  " << v.first
            << ", " << v.second.to_string()
            << endl;  
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// ap_config
// struct of ap configuration
//
///////////////////////////////////////////////////////////////////////////////

int ap_config::s_n_ap = 0;

ap_config::ap_config() {
  m_obj_id = s_n_ap++;
  generate_map_id();
  // initialize optional
  m_psi_enabled = true;
  m_psi_interval = 30;
  m_hbt_interval = 60;
  m_trans_expire_margin = 10;
}

#if 0
void ap_config::dump(ostream &os /* = std::cout */) {
  os << "ap_config#" << hex << m_obj_id << dec << ", " << m_name << endl
     << "  admin_state = " << boolalpha << m_admin_state << endl
     << "  single_step = " << boolalpha << m_single_step << endl
     << "  psi_enabled = " << boolalpha << m_psi_enabled << endl
     << "  psi_interval = " << m_psi_interval << endl
     << "  hbt_interval = " << m_hbt_interval << endl
     << "  trans_expire_margin = " << m_trans_expire_margin << endl
     << "  central_freq_khz = " << m_central_freq_khz << endl
     << "  radio_bandwidth_mhz = " << m_radio_bandwidth_mhz << endl
     << "  channel_blacklist = [" << endl;
  for (list<int>::iterator itb = m_channel_blacklist.begin();
       itb != m_channel_blacklist.end(); ++itb)
    os << (itb == m_channel_blacklist.begin() ? "    " : ", ") << (int)*itb;
  os << endl << "  ]" << endl;
  os << "  fcc_id = " << m_fcc_id << endl
     << "  ap_serial_number = " << m_serial_number << endl
     << "  category = " << m_category << endl
     << "  call_sign = " << m_call_sign << endl
     << "  meas_capabilities = [" << endl;
  for (const auto &m : m_meas_capabilities)
    os << "    " << m << endl;
  os << "  ]" << endl
     << "  radio_technology = " << m_radio_technology << endl
     << "  vendor = " << m_vendor << endl
     << "  model = " << m_model << endl
     << "  software_version = " << m_software_version << endl
     << "  hardware_version = " << m_hardware_version << endl
     << "  firmware_version = " << m_firmware_version << endl
     << "  eirp_capability = " << m_eirp_capability << endl
     << "  latitude = " << fixed << setprecision(6) << m_latitude << endl
     << "  longitude = " << fixed << setprecision(6) << m_longitude << endl
     << "  height = " << fixed << setprecision(6) << m_height << endl
     << "  height_type = " << m_height_type << endl
     << "  horizontal_accuracy = " << m_horizontal_accuracy << endl
     << "  vertical_accuracy = " << m_vertical_accuracy << endl
     << "  indoor_site = " << boolalpha << m_indoor_site << endl
     << "  antenna_azimuth = " << m_antenna_azimuth << endl
     << "  antenna_downtilt = " << m_antenna_downtilt << endl
     << "  antenna_beamwidth = " << m_antenna_beamwidth << endl
     << "  antenna_model = " << m_antenna_model << endl
     << "  groups = [" << endl;
  for (list<string>::iterator itt = m_group_types.begin(),
                              iti = m_group_ids.begin();
       itt != m_group_types.end() && iti != m_group_ids.end(); ++itt, ++iti)
    os << "    { type = " << (*itt) << ", id = " << (*iti) << " }" << endl;
  os << "  ]" << endl
     << "  cpi_signature_data," << endl
     << "    protected_header = " << m_protected_header << endl
     << "    encoded_cpi_signed_data = " << m_encoded_cpi_signed_data << endl
     << "    digital_signature = " << m_digital_signature << endl
     << "  ap_cert = " << m_ap_cert << endl
     << "  ap_key = " << m_ap_key << endl
     << "  key_passwd = " << m_key_passwd << endl
     << endl
     << "meta info:" << endl;
  
  for (auto v : get_meta())
    os << "  " << v.first
        << ", " << v.second.to_string()
        << endl;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// ap_config_v2
// struct of ap configuration
//
///////////////////////////////////////////////////////////////////////////////

int ap_config_v2::s_n_ap = 0;

ap_config_v2::ap_config_v2() {
    m_obj_id = s_n_ap++;
    generate_map_id();
    // initialize optional
    m_psi_enabled = true;
    m_psi_interval = 30;
    m_hbt_interval = 60;
    m_trans_expire_margin = 10;
}

///////////////////////////////////////////////////////////////////////////////
//
// app_config
// class of application configuration
//
///////////////////////////////////////////////////////////////////////////////

app_config::app_config() {
  m_obj_id = 0xfffful;
  generate_map_id();
  // initialize optional
  m_thread_pool_size = 1;
  m_max_req_per_msg = 1;
  m_flush_interval_ms = 1000;
  m_log_path = "./";
  m_sas_log_level = "debug";
  m_dev_log_level = "debug";
  m_con_log_level = "debug";
}

#if 0
void app_config::dump(ostream &os /* = std::cout */) {
  os << "app_config#" << hex << m_obj_id << dec << ", " << get_key() << endl
     << "  thread_pool_size = " << m_thread_pool_size << endl
     << "  max_req_per_msg = " << m_max_req_per_msg << endl
     << "  flush_interval_ms = " << m_flush_interval_ms << endl
     << "  log_path = " << m_log_path << endl
     << "  sas_log_level = " << m_sas_log_level << endl
     << "  dev_log_level = " << m_dev_log_level << endl
     << "  con_log_level = " << m_con_log_level << endl
     << endl
     << "meta info:" << endl;

  for (auto v : get_meta())
    os << "  " << v.first
        << ", " << v.second.to_string()
        << endl;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// shim
// singleton class of shim layer
//
///////////////////////////////////////////////////////////////////////////////

shim::shim() {}

shim::~shim() {}

void shim::initialize() { subscribe(this); }

int shim::insert_config(object_config_ptr cfg) {
  delete_config(cfg->get_key());

  cfg->subscribe(this);
  uint64_t id = cfg->get_map_id();
  string key = cfg->get_key();
  m_store[key] = cfg;
  m_id2key[id] = key;
  ordered_oc.push_back(cfg);
  notify("m_store", enum_change_add, reinterpret_cast<void *>(id));
  return 0;
}

int shim::delete_config(const string &key) {
  object_config_ptr cfg = find_config(key);
  if (cfg != nullptr) {
    uint64_t id = cfg->get_map_id();
    m_store.erase(key);
    notify("m_store", enum_change_delete, reinterpret_cast<void *>(id));
    m_id2key.erase(id);
    return 1;
  } else
    return 0;
}

int shim::delete_config(uint64_t id) {
  auto it = m_id2key.find(id);
  if (it != m_id2key.end())
    return delete_config(it->second);
  else
    return 0;
}

object_config_ptr shim::find_config(const string &key) {
  auto it = m_store.find(key);
  if (it != m_store.end())
    return it->second;
  else
    return nullptr;
}

object_config_ptr shim::find_config(uint64_t id) {
  auto it = m_id2key.find(id);
  if (it != m_id2key.end())
    return find_config(it->second);
  else
    return nullptr;
}

list<object_config_ptr> shim::find_all_config() {
    list<object_config_ptr> objs;
    for (auto p : m_store)
        objs.push_back(p.second);
    return objs;
}

void shim::dump(ostream &os /* = std::cout */) {
  ostringstream oss_all;
  for (const auto &o : m_store) {
    uint64_t map_id = o.second->get_map_id();
    ostringstream oss_obj;
    if (object_config::is_site(map_id))
      oss_obj << "site_config#" << o.second->get_obj_id();
    else if (object_config::is_building(map_id))
      oss_obj << "building_config#" << o.second->get_obj_id();
    else if (object_config::is_ap(map_id))
      oss_obj << "ap_config#" << o.second->get_obj_id();
    os << dec << noshowbase << setw(24) << setfill(' ') << oss_obj.str()
       << ", 0x" << hex << setw(16) << setfill('0') << o.second->get_map_id()
       << dec << ", " << o.second.get() << ", " << o.first << endl;
    o.second->dump(oss_all);
    oss_all << endl;
  }
  os << "count: " << m_store.size() << endl;
  for (const auto &k : m_id2key)
    os << "0x" << hex << noshowbase << setw(16) << setfill('0') << k.first
       << dec << " => " << k.second << endl;
  os << "count: " << m_id2key.size() << endl << endl;
  os << "detailes: " << endl << oss_all.str() << endl;
}

void shim::on_change(publisher *p, const char *var, enum change_type type,
                     void *pd /* = nullptr */) {
  cout << "received notification of " << change_types[type] << endl;

  if (string(var) == "m_store") {
    uint64_t id = reinterpret_cast<uint64_t>(pd);
    if (type == enum_change_add) {
      if (object_config::is_ap(id)) {
#if 0
                // create and start ap
                ap *c = new ap();
                c->set_cc(std::dynamic_pointer_cast<ap_config>(find_config(id)));
                c->set_timer(m_tmr);
                c->set_tls(m_tls);
                c->initialize();

                LOG_DEV_INFO("create {} from ap_config#{}, needs to register", c->get_name(), id);
#else
        cout << "create ap object from ap_config#" << id << endl;
#endif
      }
    } else if (type == enum_change_delete) {
      if (object_config::is_ap(id)) {
#if 0
                // stop ap
                list<object *> aps;
                object::get_ap_objects(aps);
                for (auto o : aps)
                {
                    ap *c = dynamic_cast<ap *>(o);
                    if (id == c->get_cc()->get_map_id())
                    {
                        LOG_DEV_WARN("delete {} from ap_config#{}, and deregister now", c->get_name(), id);
                        event *e = new event(enum_deregister_ap, 0, c->get_id());
                        e->set_hint(enum_hint_destroy);
                        c->enqueue(e);
                        break;
                    }
                }
#else
        cout << "delete ap from ap_config#" << id << endl;
#endif
      }
    }
  }
}

} // namespace project
