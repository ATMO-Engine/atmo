#ifndef Widget_HPP_
#define Widget_HPP_

#include <string>

class Widget
{
    public:
        virtual void run() = 0;
        std::string getWidgetName() { return widgetName; }

    protected:
        const std::string widgetName;
};

#endif /* !Widget_HPP_ */
