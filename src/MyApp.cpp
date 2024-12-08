#include "MyApp.h"
#include "map.h"

#define WINDOW_WIDTH  1900
#define WINDOW_HEIGHT 1000

MyApp::MyApp() {
  ///
  /// Create our main App instance.
  ///
  app_ = App::Create();

  ///
  /// Create a resizable window by passing by OR'ing our window flags with
  /// kWindowFlags_Resizable.
  ///
  window_ = Window::Create(app_->main_monitor(), WINDOW_WIDTH, WINDOW_HEIGHT,
    false, kWindowFlags_Titled | kWindowFlags_Resizable);

  ///
  /// Create our HTML overlay-- we don't care about its initial size and
  /// position because it'll be calculated when we call OnResize() below.
  ///
  overlay_ = Overlay::Create(window_, 1, 1, 0, 0);

  ///
  /// Force a call to OnResize to perform size/layout of our overlay.
  ///
  OnResize(window_.get(), window_->width(), window_->height());

  ///
  /// Load a page into our overlay's View
  ///
  overlay_->view()->LoadURL("file:///app.html");

  ///
  /// Register our MyApp instance as an AppListener so we can handle the
  /// App's OnUpdate event below.
  ///
  app_->set_listener(this);

  ///
  /// Register our MyApp instance as a WindowListener so we can handle the
  /// Window's OnResize event below.
  ///
  window_->set_listener(this);

  ///
  /// Register our MyApp instance as a LoadListener so we can handle the
  /// View's OnFinishLoading and OnDOMReady events below.
  ///
  overlay_->view()->set_load_listener(this);

  ///
  /// Register our MyApp instance as a ViewListener so we can handle the
  /// View's OnChangeCursor and OnChangeTitle events below.
  ///
  overlay_->view()->set_view_listener(this);
}

MyApp::~MyApp() {
}

void MyApp::Run() {
  app_->Run();
}

void MyApp::OnUpdate() {
  ///
  /// This is called repeatedly from the application's update loop.
  ///
  /// You should update any app logic here.
  ///
}

void MyApp::OnClose(ultralight::Window* window) {
  app_->Quit();
}

void MyApp::OnResize(ultralight::Window* window, uint32_t width, uint32_t height) {
  ///
  /// This is called whenever the window changes size (values in pixels).
  ///
  /// We resize our overlay here to take up the entire window.
  ///
  overlay_->Resize(width, height);
}

void MyApp::OnFinishLoading(ultralight::View* caller,
                            uint64_t frame_id,
                            bool is_main_frame,
                            const String& url) {
  ///
  /// This is called when a frame finishes loading on the page.
  ///
}

void MyApp::OnDOMReady(ultralight::View* caller,
                       uint64_t frame_id,
                       bool is_main_frame,
                       const String& url) {
  ///
  /// This is called when a frame's DOM has finished loading on the page.
  ///
  /// This is the best time to setup any JavaScript bindings.
  ///
  srand(time(NULL));
    // заполняет список имен
    // временно, потом буду брать из файла
    starNames.push_back("Alpha Centauri");
    starNames.push_back("Barnard's Star");
    starNames.push_back("Betelgeuse");
    starNames.push_back("Canopus");
    starNames.push_back("Capella");
    starNames.push_back("Deneb");
    starNames.push_back("Epsilon Eridani");
    starNames.push_back("Fomalhaut");
    starNames.push_back("Gacrux");
    starNames.push_back("Hadar");
    starNames.push_back("Iota Orionis");
    starNames.push_back("Jupiter");
    starNames.push_back("Kappa Ceti");
    starNames.push_back("Lambda Ceti");
    starNames.push_back("Mira");
    starNames.push_back("Naiad");
    starNames.push_back("Omega Centauri");
    starNames.push_back("Procyon");
    starNames.push_back("Rigel");
    starNames.push_back("Sigma Draconis");
    starNames.push_back("Tau Ceti");
    starNames.push_back("Upsilon Andromedae");
    starNames.push_back("Vega");
    starNames.push_back("Zeta Puppis");

    // ввод размера галактики пользователем
    int galaxySize = 100;

    // создает галактику и заполняет ее
    Galaxy galaxy(galaxySize);
    galaxy.fill();

    const std::string map = galaxy.getMap();
    const std::string strcall = "displayGalaxyMap(`" + map + "`);";
    const char* call = strcall.c_str();
    caller->EvaluateScript(call);
}

void MyApp::OnChangeCursor(ultralight::View* caller,
                           Cursor cursor) {
  ///
  /// This is called whenever the page requests to change the cursor.
  ///
  /// We update the main window's cursor here.
  ///
  window_->SetCursor(cursor);
}

void MyApp::OnChangeTitle(ultralight::View* caller,
                          const String& title) {
  ///
  /// This is called whenever the page requests to change the title.
  ///
  /// We update the main window's title here.
  ///
  window_->SetTitle(title.utf8().data());
}
