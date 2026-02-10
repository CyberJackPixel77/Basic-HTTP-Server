-----
# HTTP Server in C for Windows
-----
> IMPORTANT: This program is a **training project** and is for educational purposes only. **This program should not be used in real-world situations or as a basis for real-world projects**.
-----
[🇷🇺 Russian version](README_RU.md) | [🇺🇸 English Version](README.md)
## Contents:
* [Technical Component of the Server](#technical-components-of-the-server)
* [Server Functionality](#brief-overview-of-server-capabilities-and-functionality)
* [Request Filtering](#request-filtering-system)
* [Request Specifics](#request-specifics)
* [Technical Supplements about the Server](#technical-information-about-the-server)
* [Server Architecture (L0-L6)](#server-architecture)
* [Attitude to the Project and Advice for Searchers](#personal-attitude-to-the-project-and-advice-for-the-future)
* [Server Testing](#how-to-test-the-server)
* [Assembly project](#how-to-build-from-source)
* [Usage nuances](#important-information-for-use)
-------------
# Technical components of the server
The server is written in C and compiled in the Microsoft Visual Studio development environment. In my code, I used some MSVC-specific functions (for example, strcpy_s), so compiling the program with gcc is impossible without quirks.
The server uses the WinSock2.h library and is specific to Windows.

# Brief overview of server capabilities and functionality
My server uses HTTP/1.0 and HTTP/1.0 protocols for data transfer; HTTP/2 and HTTP/3 protocols are not supported.
> I HAVE NOT 100% implemented all the functionality and the breakdown into distinct HTTP versions. The main difference that my server has is filtering based on context and request type.

The server supports the following HTTP methods:
- GET
- HEAD
- OPTIONS

Other methods are not supported.

The main feature of my server is *content filtering* (for more details, see the chapter "[Request Filtering System](#request-filtering-system)").
The server can filter HTTP requests from the client based on what the user wants to receive and what data the user has in the request.
The server can also process file types according to MIME.
My server uses a multi-threaded system to handle multiple clients simultaneously.

# Request Filtering System
As already mentioned, the server can filter requests based on how the request was made. The current implementation allows filtering requests by:
- HTTP version
- File name/Path
- Host: field
- User-Agent
- Accept-Language

For example, if an HTTP/1.1 request is sent without the "Host:" field, the server will reject it and return a BAD_REQUEST.
This allows for the creation of rules for serving files to the client or specific response settings.
The current implementation is rather crude, but this could be fully implemented using not only blacklist and whitelist
lists for files, but also full arrays of structures with flags, access rights, and request filtering for each file.
<br>
Current filtering system:
- HTTP/1.1 -> Requires the presence of the Host: field
- The Chrome.txt file (and information about it) can only be retrieved if the request contains the 'User-Agent: Google' field.
- The Firefox.txt file (and its associated information) can only be retrieved if the request contains the 'User-Agent: Mozilla' field.
- The File1.txt file (and its associated information) can only be retrieved if the request DOES NOT CONTAIN en-US after 'Accept-Language'.
- The File.ini file (and its associated information) cannot be retrieved in any way.
- The request must contain a path beginning with the SERVER field.
- The request must not contain the %00, ////, or Linux-way path sequences.

> It's also worth noting that the absence of a particular field used for filtering will prevent the retrieval of file information. <br>The current system will immediately reject the request if it detects that a required field for filtering is missing.
---
# Request Specifics
Since the server is written for a Windows environment, I decided to slightly modify the file path queries. The current server implementation requires an *input key* (think of it like Virtual Root Directory) for file requests, the SERVER\ field, instead of the full file path. Without this field, the server will return a BAD_REQUEST. The easiest way to associate this is with the Linux root directory.
For file requests, the "SERVER\" key is used, which is then used to construct the path. (For example, SERVER\site\index.html)
This solves several problems:
- It is now impossible to descend the directory below the *input key* (via path-traversal); my design won't allow this, since it is the ROOT of the server.
- The server path can now be any path, and changing the server directory won't even be noticed by an external user. (The path is hidden behind the SERVER\ key abstraction.)
- The path to the server directory is completely hidden.

Here's an example of a request the server will accept:
`OPTIONS SERVER\random.txt HTTP/1.1\r\nHost: My-site\r\n\r\n`

----
# Technical Information about the Server
The server is written in C, uses MSVC functions, and is Windows-specific (strcpy_s, strcat_s). It uses the WinSock2.h libraries for sockets and Windows.h (WinAPI) for multithreaded functions and access to the Windows file system.
The server does not use dynamic memory; it prioritizes stack allocation. The server runs on 127.0.0.1:8080 (localhost:8080, local running on port 8080) and waits for a connection.

# Server Architecture
The server architecture is something I didn't come to right away. I arrived at the current architecture only on my second try, and I spent a long time thinking about how to improve it. The current scheme represents the code of functions divided into abstraction levels from L0 to L6, where L0 is the root layer, from which all other layers grow, gradually expanding into a tree.
This system allows for management of the Architectural Layers, as if descending downwards to more and more specific actions.
Here is a rough description of each level:
- L0 - Root node, responsible for initial server initialization. It runs the master function and initiates the client acceptance cycle.
- L1 - Master thread, the node responsible for accepting clients and initiating contact with them. For example, it controls the receipt of an HTTP request, the formation of a response, sending, and terminating the connection.
- L2 - Node responsible for request validation, accepting, creating, and sending the request.
- L3 - Level responsible for receiving data, parsing the HTTP request into a structure, creating HTTP headers, processing various methods, and more.
- L4 - The layer responsible for validating strings in HTTP requests, searching for files in the system, advanced keyword filtering, and generating a successful response from the server.
- L5 - The layer responsible for validating the Hosts field, obtaining the method from the request, determining whether to grant or deny access to content, obtaining content types, and obtaining information from the user request.
- L6 - The layer responsible for inspecting strings in requests, summarizing logic, and server peripherals.

To simplify the descriptions, here's what we get:
- L0 - Main thread, server startup
- L1 - Starting a separate thread for each client
- L2 - Sending, receiving, and calling functions to parse the HTTP request
- L3 - Parsing HTTP headers, proper termination according to the HTTP standard
- L4 - Superficial analysis of the HTTP request itself (path, content)
- L5 - Parsing the string with the requested content. Making decisions based on filter lists
- L6 - Working with strings, peripherals, residual actions, final conclusions about user intent.

I'm quite proud of this project structure (especially considering it's a learning project) because it's applicable to any "heavy" program.
The main problem is the difficulty of returning information from the lowest layers (L7) to the control layers (L2) without distorting the meaning.

-----
# Personal attitude to the project and advice for the future
Personally, this project wasn't initially my primary focus. The main goal was to prove to myself that I could write, and most importantly, understand how such a code structure is created, what it actually does, and how it
works.

From my personal perspective, I can only say that working on this project, which I thought would take me a couple of evenings (instead of a month),
I learned many important lessons, which I want to share with you.
- Enumerations in C are much better than #define for creating constant flags.
- Using enums as function types increases function predictability.
- Structures are great for passing data between functions, and typedef enums allow you to conveniently store statuses/return codes in a human-readable format without using magic numbers.
- Arrays of structures are not scary, and are actually very convenient.
- It's better to have many small and understandable functions than one that you'll get confused about.
- If you have several small functions that replicate each other's functionality, combine them.

-----
# How to test the server
For server testing, I provided the 'serverdata' folder and the servertest.ps1 script.
The serverdata folder contains *dummy files* that I use to test my server. The files don't take up much space, but they allow you to clearly see how the MIME system and request filtering work.
To automate all tests and make things easier, I wrote a small PowerShell script that tests the server using classic request patterns. These requests are specifically designed to elicit the appropriate response from the server. To test, first run the server.exe file (from any location on disk), and then run servertest.ps1. The test will complete in about a second, and you can view the results.
If you want to change the script settings, please do so. My file is just a template for testing basic filters.

# How to build from source
Since the project is written entirely for Windows and is incompatible even with gcc (without quirks), we have to use convoluted build methods.
**The easiest option is to simply open the .sln file** (This requires a full-fledged Microsoft Visual Studio of any version). Open the file -> View -> Solution Explorer -> Select main.c -> Click the green arrow at the top of the screen
(Start the local Windows debugger or the button to the right of it).

**The second option is to build the .exe file using a Makefile**. For this, you will need "**Build Tools for Visual Studio**." This is not an IDE, but a build environment. (If you have VS, you already have it.) After installation, type "Developer Command Prompt for VS (your version)" in the Start menu.
After launching, navigate to the folder where the source files are (using the cd command in the console) and type nmake. An .exe file should appear.

---
# Important information for use:
- This is NOT a FULL PRODUCT, but a test, educational sample. - The server.exe program starts a LOCAL HTTP SERVER on port 8080 (127.0.0.1:8080). To change the port, you'll have to edit the code.
- **The server initially searches for files in the path `C:\Server\serverdata`, so it's best to create a Server folder in the root of your drive, copy this repository, and use it. To CHANGE THE FILE SEARCH PATH, go to the server.h file and look for #define SERV_WORKPATH. Substitute your path there, and recompile the application.**
- The PowerShell script may generate warnings (something related to the launch policy) and won't run. Run it from the Windows console with this command: <br>
- `powershell.exe -ExecutionPolicy Bypass -File "path_to_script.ps1"
``` `
- The server can work with files other than those in the folder; you can experiment and use your own files.
---------------------------------------
