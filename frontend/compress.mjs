import { gzip } from "@gfx/zopfli";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const chunkArray = (input, size) =>
  input.reduce((resultArray, item, index) => {
    const cIndex = Math.floor(index / size);

    if (!resultArray[cIndex]) {
      resultArray[cIndex] = [];
    }

    resultArray[cIndex].push(item);

    return resultArray;
  }, []);

const addLineBreaks = (buffer) => {
  const chunks = chunkArray(buffer, 30);
  return chunks.reduce((data, chunk, index) => {
    data += chunk.join(",");
    if (index + 1 !== chunks.length) {
      data += ",\n";
    }
    return data;
  }, "");
};

gzip(
  fs.readFileSync("./dist/index.html", {
    encoding: "utf-8",
  }),
  { numiterations: 30 },
  (err, output) => {
    if (err) {
      return console.error(err);
    }

    const FILE = `#include "webgui.h"

#ifdef ENABLE_SERVER

#include <ESPAsyncWebServer.h>

const uint32_t GUI_HTML_SIZE = ${output.length};
const uint8_t GUI_HTML[] PROGMEM = {${addLineBreaks(output)}};

void startGui(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", GUI_HTML, GUI_HTML_SIZE);
  response->addHeader("Content-Encoding", "gzip");
  // the page changes with every firmware update, never serve a stale copy
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

#endif
`;

    fs.writeFileSync(path.resolve(__dirname, "../src/webgui.cpp"), FILE);
  },
);
