#include "ofApp.h"
#include "ofAppNoWindow.h"


int main()
{
    ofAppNoWindow window;
    ofSetupOpenGL(&window, 1024, 768, OF_WINDOW);
    return ofRunApp(std::make_shared<ofApp>());
}
