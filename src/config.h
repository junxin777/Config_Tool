#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <list>
#include <string>
// #include <variant>
// #include <map>
#include <unordered_set>

#include <libconfig.h++>

#include "shim.h"

using namespace libconfig;
using namespace std;

namespace project {

// aliases
using settings = libconfig::Setting;
using exception_io = libconfig::FileIOException;
using exception_parsing = libconfig::ParseException;
using exception_setting = libconfig::SettingException;

///////////////////////////////////////////////////////////////////////////////
//
// configurator
// base class of configuration file reader/writter
//
///////////////////////////////////////////////////////////////////////////////

class configurator : public Config {
public:
  configurator();
  virtual ~configurator();

  // accessories
  string get_ifname() { return m_ifn; }
  string get_ofname() { return m_ofn; }
  string get_error() { return m_error; }
  void reset_error() { m_error.clear(); }
  int get_ver() { return m_ver; }

  //
  virtual bool load_config(const string &);
  virtual bool parse_config() = 0;
  // virtual bool migrate_config(const string &ifn, const string &ofn) = 0;
  virtual bool build_config() = 0;
  virtual bool save_config(const string &);
  void reset_result_cfg();

protected:
  enum { 
    enum_ver_1 = 1, 
    enum_ver_2 = 2,
    enum_ver_3 = 3,
    enum_ver_4 = 4,
    enum_ver_latest = enum_ver_1 
  };

  string m_ifn;
  string m_ofn;
  string m_error;
  int m_ver;
  Config result_cfg;

}; // class configurator

///////////////////////////////////////////////////////////////////////////////
//
// meta_cfg
// class of generating schema of configuration file
//
///////////////////////////////////////////////////////////////////////////////

class meta_cfg : public configurator {
public:
  virtual bool parse_config();
  virtual bool migrate_config(const string &, const string &);
  virtual bool build_config();
  virtual bool save_config(const string &);

private:
  void traverse(settings &);
};

///////////////////////////////////////////////////////////////////////////////
//
// app_cfg
// class of application configuration file reader/writter
//
///////////////////////////////////////////////////////////////////////////////

class app_cfg : public configurator {
public:
  virtual bool parse_config();
  virtual bool migrate_config(const string &ifn, const string &ofn);
  virtual bool build_config();

}; // class app_cfg

///////////////////////////////////////////////////////////////////////////////
//
// ap_cfg
// class of ap configuration file reader/writter
//
///////////////////////////////////////////////////////////////////////////////

class ap_cfg : public configurator {
public:
  virtual void reset() {
    settings &root = getRoot();
    while (root.getLength() > 0)
      root.remove((unsigned int)0);
    m_ifn.clear();
    m_ofn.clear();
    // error reseted in build function
  }
  virtual bool parse_config();
  bool set_compare(settings &A, settings &B);
  virtual bool migrate_config(const string &, const string &);
  virtual bool build_config();

private:
  vector<string> added, deleted;
  struct modify {
    string name;
    int dp = 0;
    int cl = 0;
    int ap = 0;
    int val_dif = 0;
    int op_to_man = 0;
    int man_to_op = 0;
  };
  vector<modify> modified;
  unordered_set<string> man, opt;

}; // class ap_cfg

///////////////////////////////////////////////////////////////////////////////
//
// shim_cfg
// class of loading/saving libconfig file based on shim config object meta info
//
///////////////////////////////////////////////////////////////////////////////

class shim_cfg : public configurator
{
public:
  virtual bool parse_config();
  bool migrate_config(int);
  virtual bool build_config();

  list<string> get_added() { return added; }
  void set_added(list<string> i) { added = i; }

  void reset(){
    reset_error();
    settings &root = result_cfg.getRoot();
    while (root.getLength() > 0)
      root.remove((unsigned int)0);
  }

private:
  string get_parent_path(const settings &);
  void traverse(const settings &);
  void build_traverse(shim &sh);

  object_config_ptr duplicate(const object_config_ptr &, int);

  list<object_config_ptr> m_src_objs;
  list<object_config_ptr> m_dst_objs;
  meta_map *m_src_meta;
  meta_map *m_dst_meta;
  list<string> added;

}; // class shim_cfg

} // namespace project

#endif // __CONFIG_H__
