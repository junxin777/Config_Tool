#ifndef __CONST_H__
#define __CONST_H__

namespace project
{

// change type of notification
enum change_type
{
    enum_change_add,
    enum_change_delete,
    enum_change_update,
    enum_change_max
};

extern const char *change_types[];

} // namespace project

#endif // __CONST_H__
