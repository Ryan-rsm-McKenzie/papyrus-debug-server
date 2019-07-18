#pragma once

#include <functional>
#include "xse.h"

namespace DarkId::Papyrus::DebugServer

{
    class FuncTask : public ITaskDelegate
    {
        std::function<void()> m_func;
    public:
        FuncTask(std::function<void()> func) : m_func(func)
        {
        }

        virtual void Run()
        {
            m_func();
        };
    };
}