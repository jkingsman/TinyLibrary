# TinyLibrary

A self-hosted ebook library and reader for the ESP32.

See a demo here: https://www.youtube.com/watch?v=g9V4MR0gx3M

## ESP32 Code

The code in `ebook_lib_esp32_code` is intended for upload to the ESP32 and essentially sets up a reasonable performant web server, broadcasting wifi and redirecting DNS requests to point at the platform URL (http://192.168.4.0). Upcoming improvements intend to use DHCP options to provide a proper captive portal, but for now we hack it up with DNS.

It's an `ino` file intended for use with the Arduino IDE but should work on just about any ESP32.

## Run on Ubuntu 24 (e.g. Raspberry Pi Zero 2W)

See `Ubuntu 24 Static File Captive Portal.md` in this repo.

## Website

The website is served from `index.html`, linking to `reader.html` for the ePub reader.

Strictly speaking, the books are source from `books.json`, a blob providing basic information (title, author, cover, etc. about each book). However, generation of this file is automated with exports from [Calibre](https://calibre-ebook.com/), a fantastic eBook management system. Select your books, right click, "Save to disk", "Save only ePub format to disk in single folder," and save to the `./books` directory.

When the `./books` directory is populated with ePubs, covers, and `.opf` metadata files, run `./generate_book_list_and_metadata.py books/` to automatically parse the `.opf` files and generate `books.json`.

Note, there is nothing preventing you from manually populating your `books.json` or writing your own script. At the least, it requires keys for `title`, `author`, `tags`, `description`, `cover`, and `file` in an object of objects keyed on the display-able book title, although those are all free to be kept empty if desired (except for file, which is needed for download/viewing).

Once `books.json` is generated, copy all the files in the repo to a FAT32-formatted microSD card and insert it into a reader wired to the ESP32 (`3V3` or `VIN` to ESP32 `3V3` or `5V` as appropriate, `CS`/`CLK` to ESP32 GPIO 5, `MOSI` to ESP32 GPIO 23, `CLK` to ESP32 GPIO 18, `MISO` to ESP32 GPIO 19, `GND` to `GND`). Once booted, the wifi network should appear and the library is accessible at http://192.168.4.0.
