
* **anomalon.hpp / anomalon.cpp**: The primary entry point and initialization layer for your web engine.
* **Anomalon::Core**: The root C++ namespace housing your Boost.Beast HTTP/WebSocket listeners.
* **Anomalon::Reactor**: A great name for the Boost.Asio io_context loop handling asynchronous events.
* **Anomalon::Binding**: The bridge layer mapping your C++ performance classes into the Lua state.
* **anomalon.lua**: The main user-facing script where developers write their high-level server logic.


      anomalon/
      ├── cmake/
      ├── include/
      │   └── anomalon/
      │       ├── engine.hpp
      │       ├── session.hpp
      │       └── lua_binding.hpp
      ├── src/
      │   ├── main.cpp
      │   └── engine.cpp
      ├── scripts/
      │   └── server.lua
      └── CMakeLists.txt

