#ifndef SERVICEHELPER_H
#define SERVICEHELPER_H

#include <unordered_map>
#include <functional>
#include "messagepackage.h"

class ServiceHelper
{
public:
    static ServiceHelper* instance();
private:
    ServiceHelper();
};

#endif // SERVICEHELPER_H
