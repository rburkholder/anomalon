## Anomalon

__Anomalon__ / Uh-nom-uh-lon : A theoretical nuclear particle fragment that possesses an uncharacteristically large collision area. It is a highly scientific, smooth-sounding noun that directly borrows the prefix of anomalistic.

The repository is non-active.  It is a one-time-use repository recording the LLM interactions to:

* Find a name for web engine
* Discover the skeleton requirements for building a web server by
   * integrating Lua (using Sol2) for
   * HTML code generation into a
   * C++ base with Boost Beast, using
   * co-routines for handling multiple simultaneous connections

The engine can be found at [Repertory/Anomalon](https://github.com/rburkholder/repertory/tree/master/Anomalon)

* listen.cpp - main port listening logic to support sessions and requests, integrates ssl and web-socket ability
* handle_methods.cpp - logic for handling specific request types (generic and lua based)
* sol_lua.cpp - small object to encapsulate a Lua instance (luajit)

Ultimately will integrate into:

* [Tangle](https://github.com/rburkholder/tangle) - primary public web site server
* [Apparition](https://github.com/rburkholder/apparition) - IoT/MQTT automation server
* [TradeFrame](https://github.com/rburkholder/trade-frame) - Market Analysis and Automated Trading

Note:
* code compiles with gcc
* executable does run but is missing the execution loop
