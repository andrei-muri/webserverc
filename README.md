# Web server in C

This is a HTTP server implemented in C, designed to run on ***Linux*** machines.

## Features
- Serves **static files** (*HTML, CSS, JS, PNG, JPG, JPEG, GIF, ICO*) stored in a folder called **html**
- Supports root route (/ -> */index.html*)
- A dummy web page that includes html, css, js and a jpg file.

> [!IMPORTANT]
> In this version, the server only receives connections from localhost.

## Running the server on Linux\
1. Clone the repository
```bash
git clone https://github.com/andrei-muri/webserverc.git
```
2. Compile the code
```
gcc httpd.c -o httpd
```
3. Run the server
```
./httpd
```

> [!WARNING]
> Binding errors can appear at running if port 8080 is used by another process. A feature that will enable dinamic porting will be added in future versions.

4. Make a http request through the browser (Chrome, Safari, Firefox, Brave etc.)
```
http://localhost:8080/
```

## Further development
- [ ] The user is able to select the port to which the process listens
- [ ] Implement multithreading to handle multiple simultaneous connections
- [ ] Improve compatibility for macOS and Windows (replace `sendfile()` with a cross-platform alternative).
- [ ] Enable connections from the outside

## Author
Created by ***Muresan Andrei***  
[GitHub](https://github.com/andrei-muri)
[LinkedIn](https://www.linkedin.com/in/andrei-muresan-muri/)
