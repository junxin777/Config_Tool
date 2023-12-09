#ifndef __SHIM_H__
#define __SHIM_H__

#include <any>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <tuple>
#include <string>
#include <vector>

#include "const.h"

using namespace project;
using namespace std;

namespace project {

class accessor;

enum {
  enum_ver_1 = 1,
  enum_ver_2 = 2,
  enum_ver_latest = enum_ver_1
};

enum var_trait_t {
  enum_var_required,
  enum_var_optional,
  enum_var_composed,
  enum_var_undefined
};

struct meta_t {
  string m_node;
  string m_type;
  enum var_trait_t m_trait;
  string to_string() {
    ostringstream oss;
    oss << "type = " << m_type
        << ", trait = " << (m_trait == enum_var_required ? "required" : "optional");
    return oss.str();
  }
  bool operator==(const meta_t &rhs) const {
    return (m_node == rhs.m_node && m_type == rhs.m_type && m_trait == rhs.m_trait);
  }
};

struct member_t {
  void *m_var;
  accessor *m_xetter;
};

typedef map<string, meta_t> meta_map;
typedef map<string, member_t> member_map;

#define decl_mem_var(t, v)                                            \
  protected:                                                          \
    t m_##v;                                                          \
  public:                                                             \
    const t &get_##v() { return m_##v; };                             \
    void set_##v(const t &__) {                                       \
      m_##v = __;                                                     \
      notify(#v, enum_change_update);                                 \
    }

#define begin_def_vars()                                              \
  public:                                                             \
    static meta_map &get_meta() {                                     \
      static meta_map s_vars;                                         \
      return s_vars;                                                  \
    }                                                                 \
    virtual meta_map &get_meta_info() {                               \
      return get_meta();                                              \
    }                                                                 \
  protected:                                                          \
    static bool &is_inited() {                                        \
      static bool s_inited = false;                                   \
      return s_inited;                                                \
    }                                                                 \
    void init_vars_list() {                                           \

#define def_required_ex(t, v, n) {                                    \
      if (!is_inited()) {                                             \
        meta_t m = {                                                  \
          std::move(string(#n)),                                      \
          std::move(string(#t)),                                      \
          enum_var_required };                                        \
        get_meta()[#v] = m;                                           \
      }                                                               \
      member_t b = {                                                  \
        &m_##v,                                                       \
        new xetter<t>() };                                            \
      get_members()[#v] = b; }

#define def_optional_ex(t, v, n) {                                    \
      if (!is_inited()) {                                             \
        meta_t m = {                                                  \
          std::move(string(#n)),                                      \
          std::move(string(#t)),                                      \
          enum_var_optional };                                        \
        get_meta()[#v] = m;                                           \
      }                                                               \
      member_t b = {                                                  \
        &m_##v,                                                       \
        new xetter<t>() };                                            \
      get_members()[#v] = b; }

#define def_composed_ex(t, v, n) {                                    \
      if (!is_inited()) {                                             \
        meta_t m = {                                                  \
          std::move(string(#n)),                                      \
          std::move(string(#t)),                                      \
          enum_var_composed };                                        \
        get_meta()[#v] = m;                                           \
      }                                                               \
      member_t b = {                                                  \
        &m_##v,                                                       \
        new xetter<t>() };                                            \
      get_members()[#v] = b; }

#define def_required(t, v)          def_required_ex(t, v, v)
#define def_optional(t, v)          def_optional_ex(t, v, v)
#define def_composed(t, v)          def_composed_ex(t, v, v)

#define end_def_vars()                                                \
  is_inited() = true;                                                 \
    };

// forward declaraction
class publisher;

class object_config;
typedef shared_ptr<object_config> object_config_ptr;

// site declaraction, typedef and alias
class site_config;
typedef shared_ptr<site_config> site_config_ptr;
using site_config_v1 = site_config;
using site_config_v1_ptr = site_config_ptr;
using site_config_v2 = site_config;
using site_config_v2_ptr = site_config_ptr;

// building declaraction, typedef and alias
class building_config;
typedef shared_ptr<building_config> building_config_ptr;
using building_config_v1 = building_config;
using building_config_v1_ptr = building_config_ptr;
using building_config_v2 = building_config;
using building_config_v2_ptr = building_config_ptr;

// ap declaraction, typedef and alias
class ap_config;
class ap_config_v2;
typedef shared_ptr<ap_config> ap_config_ptr;
typedef shared_ptr<ap_config_v2> ap_config_v2_ptr;
using ap_config_v1 = ap_config;
using ap_config_v1_ptr = ap_config_ptr;

// app declaraction, typedef
class app_config;
typedef shared_ptr<app_config> app_config_ptr;

///////////////////////////////////////////////////////////////////////////////
//
// subscriber
// class of subscriber
//
///////////////////////////////////////////////////////////////////////////////

class subscriber {
public:
  subscriber() {}
  virtual ~subscriber() {}

  virtual void on_change(publisher *, const char *, enum change_type,
                         void * = nullptr) = 0;

}; // class subscriber

///////////////////////////////////////////////////////////////////////////////
//
// publisher
// class of publisher
//
///////////////////////////////////////////////////////////////////////////////

class publisher {
public:
  publisher() : m_enabled(true){};
  virtual ~publisher(){};

  bool get_enabled() { return m_enabled; }
  void set_enabled(bool e) { m_enabled = e; }

  void subscribe(subscriber *s) {
    unsubscribe(s);
    m_subscribers.push_back(s);
  }
  void unsubscribe(subscriber *s) { m_subscribers.remove(s); }

  void notify(const char *var, enum change_type ct, void *pd = nullptr) {
    if (m_enabled)
      for (auto s : m_subscribers)
        s->on_change(this, var, ct, pd);
    m_enabled = true;
  }

private:
  bool m_enabled;
  list<subscriber *> m_subscribers;

}; // class publisher

///////////////////////////////////////////////////////////////////////////////
//
// accessor
// base class of configuration getter and setter
//
///////////////////////////////////////////////////////////////////////////////

class accessor {
  public:
    virtual ~accessor(){};
    virtual void get(void *, std::any &) = 0;
    virtual void set(void *, const std::any &) = 0;
};

template <typename T>
class xetter : public accessor {
  public:
    virtual void get(void *param, std::any &value) {
      value = *static_cast<T *>(param);
    }
    virtual void set(void *param, const std::any &value) {
      // *dereference
      *static_cast<T *>(param) = std::any_cast<T>(value);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// object_config
// base class of object configuration
//
///////////////////////////////////////////////////////////////////////////////

class object_config : public publisher {
public:
  virtual string get_key() = 0;
  virtual meta_map &get_meta_info() = 0;
  virtual void dump(ostream & = std::cout);
  virtual void dump_meta(ostream & = std::cout);

  static bool is_site(uint64_t map_id) { return (map_id >> enum_shift_site) != 0; }
  static bool is_building(uint64_t map_id) { return ((map_id >> enum_shift_building) & 0xffff) != 0; }
  static bool is_ap(uint64_t map_id) { return (map_id & 0xfffffffful) != 0; }

  static object_config_ptr create_site_config(int);
  static object_config_ptr create_building_config(int);
  static object_config_ptr create_ap_config(int);
  
  uint64_t get_map_id() { return m_map_id; }
  member_map &get_members() { return m_members; }

  bool set(const string &, const std::any &);
  bool get(const string &, std::any &);

  decl_mem_var(uint64_t, obj_id);
  decl_mem_var(int, ver);

protected:
  object_config() : m_map_id(0){};
  object_config(const object_config &);
  object_config &operator=(const object_config &);
  virtual ~object_config() {
    // release members xetter object
    for (auto b : m_members) {
      delete b.second.m_xetter;
    }
  }

  enum bit_shift_t {
    enum_shift_site = 48,
    enum_shift_building = 32,
    enum_shift_ap = 0
  };

  virtual void generate_map_id() = 0;

  uint64_t m_map_id;
  member_map m_members;

}; // class object_config

///////////////////////////////////////////////////////////////////////////////
//
// site_config
// class of site configuration
//
///////////////////////////////////////////////////////////////////////////////

class site_config : public object_config {
public:
  static site_config_ptr create() {
    std::shared_ptr<site_config> ptr(new site_config());
    if (ptr)
      ptr->init_vars_list();
    return ptr;
  }

  virtual string get_key() { return m_name; }
#if 0
  virtual void dump(ostream & = std::cout);
#endif

  decl_mem_var(string, name);
  decl_mem_var(int32_t, ap_mode);
  decl_mem_var(string, spectrum_controller_host);

  decl_mem_var(string, dns_cache);
  decl_mem_var(long, dns_interval);
  decl_mem_var(int, local_port);
  decl_mem_var(long, maxage_conn);
  decl_mem_var(long, conn_timeout);
  decl_mem_var(long, ip_resolve);
  decl_mem_var(string, interface);
  decl_mem_var(long, tcp_keepalive_idle);
  decl_mem_var(long, tcp_keepalive_interval);
  decl_mem_var(bool, tls_debug);

  begin_def_vars()
    def_required(string, name)
    def_optional(int32_t, ap_mode)
    def_optional(string, spectrum_controller_host)
    def_optional_ex(string, dns_cache, tls.dns_cache)
    def_optional_ex(long, dns_interval, tls.dns_interval)
    def_optional_ex(int, local_port, tls.local_port)
    def_optional_ex(long, maxage_conn, tls.maxage_conn)
    def_optional_ex(long, conn_timeout, tls.conn_timeout)
    def_optional_ex(long, ip_resolve, tls.ip_resolve)
    def_optional_ex(string, interface, tls.interface)
    def_optional_ex(long, tcp_keepalive_idle, tls.tcp_keepalive_idle)
    def_optional_ex(long, tcp_keepalive_interval, tls.tcp_keepalive_interval)
    def_optional_ex(bool, tls_debug, tls.tls_debug)
  end_def_vars()

protected:
  site_config();
  site_config(const site_config &);
  site_config &operator=(const site_config &);

  virtual void generate_map_id() {
    m_map_id = (uint64_t)(m_obj_id + 1) << enum_shift_site;
  }

  static int s_n_site;

}; // class site_config

///////////////////////////////////////////////////////////////////////////////
//
// building_config
// class of building configuration
//
///////////////////////////////////////////////////////////////////////////////

class building_config : public object_config {
public:
  static building_config_ptr create() {
    std::shared_ptr<building_config> ptr(new building_config());
    if (ptr)
        ptr->init_vars_list();
    return ptr;
  }

  virtual string get_key() { return m_name; }
#if 0
  virtual void dump(ostream & = std::cout);
#endif

  decl_mem_var(string, name);
  decl_mem_var(string, site_name);
  decl_mem_var(string, sas_url);
  decl_mem_var(string, user_id);
  decl_mem_var(string, ca_path);
  decl_mem_var(string, root_ca);
  decl_mem_var(string, sas_crl);

  begin_def_vars()
    def_required(string, name);
    // NOTE: do not define composed field
    def_required(string, site_name)
    def_required(string, sas_url);
    def_required(string, user_id);
    def_required(string, ca_path);
    def_optional(string, root_ca);
    def_optional(string, sas_crl);
  end_def_vars()

protected:
  building_config();
  building_config(const building_config &);
  building_config &operator=(const building_config &);

  virtual void generate_map_id() {
    m_map_id = (uint64_t)(m_obj_id + 1) << enum_shift_building;
  }

  static int s_n_building;

}; // class building_config

///////////////////////////////////////////////////////////////////////////////
//
// ap_config
// class of ap configuration
//
///////////////////////////////////////////////////////////////////////////////

class ap_config : public object_config {
public:
  static ap_config_ptr create() {
    std::shared_ptr<ap_config> ptr(new ap_config());
    if (ptr)
      ptr->init_vars_list();
    return ptr;
  }

  virtual string get_key() { return m_fcc_id + ":" + m_serial_number; }
#if 0
  virtual void dump(ostream & = std::cout);
#endif

  decl_mem_var(string, name);
  decl_mem_var(string, site_name);
  decl_mem_var(string, building_name);

  decl_mem_var(bool, admin_state);
  decl_mem_var(bool, single_step);
  decl_mem_var(bool, persistent);
  decl_mem_var(bool, psi_enabled);
  decl_mem_var(int, psi_interval);
  decl_mem_var(int, hbt_interval);
  decl_mem_var(int, trans_expire_margin);
  decl_mem_var(unsigned, central_freq_khz);
  decl_mem_var(unsigned, radio_bandwidth_mhz);
  decl_mem_var(list<int>, channel_blacklist);

  decl_mem_var(string, fcc_id);
  decl_mem_var(string, serial_number);
  decl_mem_var(string, category);
  decl_mem_var(string, call_sign);
  decl_mem_var(list<string>, meas_capabilities);
  decl_mem_var(string, radio_technology);
  decl_mem_var(string, vendor);
  decl_mem_var(string, model);
  decl_mem_var(string, software_version);
  decl_mem_var(string, hardware_version);
  decl_mem_var(string, firmware_version);
  decl_mem_var(int, eirp_capability);
  decl_mem_var(double, latitude);
  decl_mem_var(double, longitude);
  decl_mem_var(double, height);
  decl_mem_var(string, height_type);
  decl_mem_var(double, horizontal_accuracy);
  decl_mem_var(double, vertical_accuracy);
  decl_mem_var(bool, indoor_site);
  decl_mem_var(int, antenna_azimuth);
  decl_mem_var(int, antenna_downtilt);
  decl_mem_var(int, antenna_gain);
  decl_mem_var(int, antenna_beamwidth);
  decl_mem_var(string, antenna_model);
  decl_mem_var(list<string>, group_types);
  decl_mem_var(list<string>, group_ids);
  decl_mem_var(string, protected_header);
  decl_mem_var(string, encoded_cpi_signed_data);
  decl_mem_var(string, digital_signature);

  decl_mem_var(string, ap_cert);
  decl_mem_var(string, ap_key);
  decl_mem_var(string, key_passwd);

  begin_def_vars()
    // NOTE: do not define composed field
    def_required(string, name)
    def_required(string, site_name)
    def_required(string, building_name)

    def_required(bool, admin_state)
    def_required(bool, single_step)
    def_optional(bool, persistent)
    def_optional(bool, psi_enabled)
    def_optional(int, psi_interval)
    def_optional(int, hbt_interval)
    def_optional(int, trans_expire_margin)
    def_required(unsigned, central_freq_khz)
    def_required(unsigned, radio_bandwidth_mhz)
    def_required(list<int>, channel_blacklist)

    def_required(string, fcc_id)
    def_required(string, serial_number)
    def_required(string, category)
    def_required(string, call_sign)
    def_required(list<string>, meas_capabilities)
    def_required(string, radio_technology)
    def_required(string, vendor)
    def_required(string, model)
    def_required(string, software_version)
    def_required(string, hardware_version)
    def_required(string, firmware_version)
    def_required(int, eirp_capability)
    def_required(double, latitude)
    def_required(double, longitude)
    def_required(double, height)
    def_required(string, height_type)
    def_required(double, horizontal_accuracy)
    def_required(double, vertical_accuracy)
    def_required(bool, indoor_site)
    def_required(int, antenna_azimuth)
    def_required(int, antenna_downtilt)
    def_required(int, antenna_gain)
    def_required(int, antenna_beamwidth)
    def_required(string, antenna_model)
    def_required_ex(list<string>, group_types, groups.[%d].type)
    def_required_ex(list<string>, group_ids, groups.[%d].id)
    def_optional_ex(string, protected_header, cpi_signature_data.protected_header)
    def_optional_ex(string, encoded_cpi_signed_data, cpi_signature_data.encoded_cpi_signed_data)
    def_optional_ex(string, digital_signature, cpi_signature_data.digital_signature)

    def_optional(string, ap_cert)
    def_optional(string, ap_key)
    def_optional(string, key_passwd)
  end_def_vars()

protected:
  ap_config();
  ap_config(const ap_config &);
  ap_config &operator=(const ap_config &);

  virtual void generate_map_id() {
    m_map_id = (uint64_t)(m_obj_id + 1) << enum_shift_ap;
  }

  static int s_n_ap;

}; // class ap_config

///////////////////////////////////////////////////////////////////////////////
//
// ap_config_v2
// class of ap configuration
//
///////////////////////////////////////////////////////////////////////////////
class ap_config_v2 : public object_config {
public:
  static ap_config_v2_ptr create() {
    std::shared_ptr<ap_config_v2> ptr(new ap_config_v2());
    if (ptr)
      ptr->init_vars_list();
    return ptr;
  }
  
  virtual string get_key() { return m_fcc_id + ":" + m_serial_number; }

  decl_mem_var(string, name);
  decl_mem_var(string, site_name);
  decl_mem_var(string, building_name);

  decl_mem_var(bool, admin_state);
  decl_mem_var(bool, single_step);
  decl_mem_var(bool, persistent);
  decl_mem_var(bool, psi_enabled);
  decl_mem_var(int, psi_interval);
  decl_mem_var(int, hbt_interval);
  decl_mem_var(int, trans_expire_margin);
  decl_mem_var(unsigned, central_freq_khz);
  decl_mem_var(unsigned, radio_bandwidth_mhz);
  decl_mem_var(list<int>, channel_blacklist);

  decl_mem_var(string, fcc_id);
  decl_mem_var(string, serial_number);
  decl_mem_var(string, category);
  decl_mem_var(string, call_sign);
  decl_mem_var(list<string>, meas_capabilities);
  decl_mem_var(string, radio_technology);
  decl_mem_var(string, vendor);
  decl_mem_var(string, model);
  decl_mem_var(string, software_version);
  decl_mem_var(string, hardware_version);
  decl_mem_var(string, firmware_version);
  decl_mem_var(int, eirp_capability);
  decl_mem_var(double, latitude);
  decl_mem_var(double, longitude);
  decl_mem_var(double, height);
  decl_mem_var(string, height_type);
  decl_mem_var(double, horizontal_accuracy);
  decl_mem_var(double, vertical_accuracy);
  decl_mem_var(bool, indoor_site);
  decl_mem_var(int, antenna_azimuth);
  decl_mem_var(int, antenna_downtilt);
  decl_mem_var(int, antenna_gain);
  decl_mem_var(int, antenna_beamwidth);
  decl_mem_var(string, antenna_model);
  decl_mem_var(list<string>, group_types);
  decl_mem_var(list<string>, group_ids);
  decl_mem_var(string, protected_header);
  decl_mem_var(string, encoded_cpi_signed_data);
  decl_mem_var(string, digital_signature);

  decl_mem_var(string, ap_cert);
  decl_mem_var(string, ap_key);
  //decl_mem_var(string, key_passwd);
  decl_mem_var(string, key_password);
  decl_mem_var(string, kkk);

  begin_def_vars()
    def_composed(string, name)
    def_composed(string, site_name)
    def_composed(string, building_name)

    def_required(bool, admin_state)
    def_required(bool, single_step)
    def_optional(bool, persistent)
    def_optional(bool, psi_enabled)
    def_optional(int, psi_interval)
    def_optional(int, hbt_interval)
    def_optional(int, trans_expire_margin)
    def_required(unsigned, central_freq_khz)
    def_required(unsigned, radio_bandwidth_mhz)
    def_required(list<int>, channel_blacklist)

    def_required(string, fcc_id)
    def_required(string, serial_number)
    def_required(string, category)
    def_required(string, call_sign)
    def_required(list<string>, meas_capabilities)
    def_required(string, radio_technology)
    def_required(string, vendor)
    def_required(string, model)
    def_required(string, software_version)
    def_required(string, hardware_version)
    def_required(string, firmware_version)
    def_required(int, eirp_capability)
    def_required(double, latitude)
    def_required(double, longitude)
    def_required(double, height)
    def_required(string, height_type)
    def_required(double, horizontal_accuracy)
    def_required(double, vertical_accuracy)
    def_required(bool, indoor_site)
    def_required(int, antenna_azimuth)
    def_required(int, antenna_downtilt)
    def_required(int, antenna_gain)
    def_required(int, antenna_beamwidth)
    def_required(string, antenna_model)
    def_required_ex(list<string>, group_types, groups.[%d].type)
    def_required_ex(list<string>, group_ids, groups.[%d].id)
    // modified node path, removed outer group
    def_optional(string, protected_header)
    def_optional(string, encoded_cpi_signed_data)
    def_optional(string, digital_signature)

    // modified trait
    def_required(string, ap_cert)
    def_required(string, ap_key)
    // deleted
    //def_optional(string, key_passwd)
    // added
    def_required(string, key_password)
    def_required(string, kkk)
  end_def_vars()

protected:
  ap_config_v2();
  ap_config_v2(const ap_config_v2 &);
  ap_config_v2 &operator=(const ap_config_v2 &);

  virtual void generate_map_id() { m_map_id = (uint64_t)(m_obj_id + 1) << enum_shift_ap; }

  static int s_n_ap;
};

///////////////////////////////////////////////////////////////////////////////
//
// app_config
// class of application configuration
//
///////////////////////////////////////////////////////////////////////////////

class app_config : public object_config {
public:
  static app_config_ptr create() {
    std::shared_ptr<app_config> ptr(new app_config());
    if (ptr)
        ptr->init_vars_list();
    return ptr;
  }

  virtual string get_key() { return "app_config"; }
#if 0
  virtual void dump(ostream & = std::cout);
#endif

  decl_mem_var(unsigned, thread_pool_size);
  decl_mem_var(unsigned, max_req_per_msg);
  decl_mem_var(unsigned, flush_interval_ms);
  decl_mem_var(string, log_path);
  decl_mem_var(string, sas_log_level);
  decl_mem_var(string, dev_log_level);
  decl_mem_var(string, con_log_level);

  begin_def_vars()
    def_optional(unsigned, thread_pool_size);
    def_optional(unsigned, max_req_per_msg);
    def_optional(unsigned, flush_interval_ms);
    def_optional(string, log_path);
    def_optional(string, sas_log_level);
    def_optional(string, dev_log_level);
    def_optional(string, con_log_level);
  end_def_vars()

protected:
  app_config();
  app_config(const app_config &);
  app_config &operator=(const app_config &);

  virtual void generate_map_id() {
    m_map_id = (uint64_t)m_obj_id << enum_shift_site;
  }

}; // class app_config

// backwards compatible
using site_config_v1 = site_config;
using building_config_v1 = building_config;
using ap_config_v1 = ap_config;

///////////////////////////////////////////////////////////////////////////////
//
// shim
// singleton class of shim layer
//
///////////////////////////////////////////////////////////////////////////////

class shim : public publisher, public subscriber {
public:
  // access singleton instance of shim class, thread safe
  static shim &instance() {
    static shim s_instance;
    return s_instance;
  }

  // init
  void initialize();

  // config store operation helpers
  int insert_config(object_config_ptr);
  int delete_config(const string &);
  int delete_config(uint64_t);
  object_config_ptr find_config(const string &);
  object_config_ptr find_config(uint64_t);
  list<object_config_ptr> find_all_config();

  void dump(ostream & = std::cout);

  // access key and id stores in shim
  map<uint64_t, string> get_id_key() { return m_id2key; }
  map<string, object_config_ptr> get_key_obj() { return m_store; }
  vector<object_config_ptr> get_ordered_oc() { return ordered_oc; }
  void clear_ordered_oc() { ordered_oc.clear(); }

protected:
  virtual void on_change(publisher *, const char *, enum change_type,
                         void * = nullptr);

private:
  shim();
  virtual ~shim();
  shim(const shim &);
  shim &operator=(const shim &);

  map<uint64_t, string> m_id2key;
  map<string, object_config_ptr> m_store;
  vector<object_config_ptr> ordered_oc;

}; // class shim

} // namespace project

#endif // __SHIM_H__
