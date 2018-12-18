[@bs.config {no_export: no_export}];
[@bs.val] external document: 'a = "";
[@bs.val] external window: 'a = "";

document##body##style##margin #= "0";

/* pale imessage bubble */
/* let foregroundColor = "#5da9f5"; */
/* darker imessage bubble */
let foregroundColor = "#317cf7";
let backgroundColor = "white";
let lineWidth = 3;

let console = document##createElement("div");
console##style##maxWidth #= "300px";
console##style##maxHeight #= "150px";
console##style##overflow #= "scroll";
console##style##webkitOverflowScrolling #= "touch";
console##style##color #= foregroundColor;
console##style##position #= "absolute";
console##style##bottom #= "30px";
console##style##right #= "30px";
console##style##margin #= "0px";
console##style##fontFamily #= "-apple-system, BlinkMacSystemFont, sans-serif";
console##style##transition #= "all 0.2s";
console##style##zIndex #= 999;
document##body##appendChild(console);

let log = msg => {
  let node = document##createElement("div");
  node##innerText #= Js.Json.stringifyAny(msg);
  ignore(console##prepend(node));
};
log("console here");

let (width, height) = (window##innerWidth, window##innerHeight);

let canvases = document##createElement("div");
canvases##width #= width;
canvases##height #= height;
canvases##style##position #= "absolute";
document##body##appendChild(canvases);

let staticCanvas = document##createElement("canvas");
staticCanvas##width #= width;
staticCanvas##height #= height;
staticCanvas##style##position #= "absolute";
canvases##appendChild(staticCanvas);

let staticContext = staticCanvas##getContext("2d");
staticContext##fillStyle #= backgroundColor;
staticContext##fillRect(0, 0, width, height);

let drawingCanvas = document##createElement("canvas");
drawingCanvas##width #= width;
drawingCanvas##height #= height;
drawingCanvas##style##position #= "absolute";
canvases##appendChild(drawingCanvas);


next: implement commit workflow #2
- selectively commit (pixie dust)


let drawingContext = drawingCanvas##getContext("2d");
drawingContext##fillStyle #= "lightgrey";
drawingContext##fillRect(100, 100, width, height);

type touching =
  | Down(float)
  | Up(float)
  | First;
let touching = ref(First);
type point = {
  x: int,
  y: int,
};
let strokes: ref(Js.Dict.t(array(point))) = ref(Js.Dict.empty());
let objectsOnScene = [||];
let objectTypes = [|Objs.obj1|];
/* TODO: Objs.obj1 has too many points */

let staticCanvasTouchStart = e => {
  /* e##preventDefault(); */

  log("static");

  touching := Down(Js.Date.now());
};

let drawingCanvasTouchStart = e => {
  /* e##preventDefault(); */

  log("drawing");
  touching := Down(Js.Date.now());
};

canvases##addEventListener("touchstart", e => {
  e##preventDefault();

  if (e##target === drawingCanvas) {
    drawingCanvasTouchStart(e);
  } else if (e##target === staticCanvas) {
    staticCanvasTouchStart(e);
  } else {
    log("nothing");
  };
});

let distance = (p1, p2) => {
  let x = p1.x - p2.x;
  let y = p1.y - p2.y;
  Js.Math.sqrt(float_of_int(x * x + y * y));
};

let drawingCanvasTouchEnd = e => {
  e##preventDefault();

  switch (touching^) {
  | First
  | Up(_) => ()
  | Down(startTime) =>
    let ellapsedTime = Js.Date.now() -. startTime;
    let touchesRemaining = Js.Array.from(e##targetTouches)->Js.Array.length;
    let maxTimeForCountingSomethingAsConfirm = 130.;
    if (ellapsedTime < maxTimeForCountingSomethingAsConfirm
        && touchesRemaining == 0) {
      let shortDistanceThreshold = 10.;
      let allLiftedTouchesDrewShortStrokes =
        Js.Array.every(
          touch => {
            let points =
              Js.Dict.get(strokes^, Js.String.make(touch##identifier));
            switch (points) {
            | None => true
            | Some([||] | [|_|]) => true
            | Some(points) =>
              let firstPoint = points[0];
              let lastPoint = points[Js.Array.length(points) - 1];
              let dist = distance(firstPoint, lastPoint);
              dist < shortDistanceThreshold;
            };
          },
          Js.Array.from(e##changedTouches),
        );
      if (allLiftedTouchesDrewShortStrokes) {
        let lineWidth = lineWidth + 1;
        staticContext##lineWidth #= lineWidth;
        Js.Array.forEach(
          points =>
            Js.Array.forEachi(
              (point, i) =>
                if (i == 0) {
                  staticContext##fillRect(
                    point.x + 10,
                    point.y + 10,
                    lineWidth,
                    lineWidth,
                  );
                } else {
                  let prevPoint = points[i - 1];
                  staticContext##beginPath();
                  staticContext##moveTo(prevPoint.x + 10, prevPoint.y + 10);
                  staticContext##lineTo(point.x + 10, point.y + 10);
                  staticContext##stroke();
                  staticContext##closePath();
                },
              points,
            ),
          Js.Dict.values(strokes^),
        );

        strokes := Js.Dict.empty();
      };
    } else {
      ();
        /* nothing happening here yet */
    };
  };
  touching := Up(Js.Date.now());
};

canvases#addEventListener("touchend", e => {
  e##preventDefault();
  if (e##target === drawingCanvas) {
    drawingCanvasTouchEnd(e);
  };
});

let drawingCanvasTouchMove = e => {
  e##preventDefault();

  staticContext##fillStyle #= foregroundColor;
  staticContext##strokeStyle #= foregroundColor;
  staticContext##lineWidth #= lineWidth;
  staticContext##globalAlpha #= 1;

  Js.Array.forEachi(
    (touch, i) => {
      let point = {x: touch##clientX, y: touch##clientY};
      let id = Js.String.make(e##targetTouches[i]##identifier);
      let newPoints =
        switch (Js.Dict.get(strokes^, id)) {
        | None =>
          staticContext##fillRect(point.x, point.y, lineWidth, lineWidth);
          [|point|];
        | Some([||]) => raise(Invalid_argument("impossible"))
        | Some(points) =>
          let prevPoint = points[Js.Array.length(points) - 1];
          staticContext##beginPath();
          staticContext##moveTo(prevPoint.x, prevPoint.y);
          staticContext##lineTo(point.x, point.y);
          staticContext##stroke();
          staticContext##closePath();
          Js.Array.push(point, points)->ignore;
          points;
        };
      Js.Dict.set(strokes^, id, newPoints);
    },
    Js.Array.from(e##touches),
  );
};

canvases##addEventListener("touchmove", e => {
  e##preventDefault();

  if (e##target === drawingCanvas) {
    drawingCanvasTouchMove(e);
  };
});
