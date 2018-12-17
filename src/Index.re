[@bs.config {no_export: no_export}];
[@bs.val] external document: 'a = "";
[@bs.val] external window: 'a = "";

document##body##style##margin #= "0";

let foregroundColor = "black";
let backgroundColor = "white";

let console = document##createElement("div");
console##style##width #= "300px";
console##style##maxHeight #= "120px";
console##style##overflow #= "scroll";
console##style##webkitOverflowScrolling #= "touch";
console##style##color #= foregroundColor;
console##style##position #= "absolute";
console##style##bottom #= "0px";
console##style##margin #= "0px";
console##style##fontFamily #= "-apple-system, BlinkMacSystemFont, sans-serif";
document##body##appendChild(console);

let log = msg => {
  let node = document##createElement("div");
  node##innerText #= Js.Json.stringifyAny(msg);
  ignore(console##prepend(node));
};

let canvas = document##querySelector("#index");
let (width, height) = (window##innerWidth, window##innerHeight);
canvas##width #= width;
canvas##height #= height;

let context = canvas##getContext("2d");
context##fillStyle #= backgroundColor;
context##fillRect(0, 0, width, height);

let previousDrawing = ref(None);
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
let maxTimeForCountingSomethingAsTouch = 150.;
let objectsOnScene = [||];
let objectTypes = [|Objs.obj1|];
/* next: Objs.obj1 has too many points */

canvas##addEventListener("touchstart", e => {
  e##preventDefault();
  touching := Down(Js.Date.now());
  /* Js.Array.push([||], strokes)->ignore; */
  /* Js.Array.forEach(
       touch => {
         Js.Dict.set(fingers, Js.String.make(touch##identifier),)
         fingers[touchID] = Some(fingerID);

         fingerID := fingerID^ + 1;
       },
       Js.Array.from(e##targetTouches),
     ); */
});

canvas##addEventListener("touchend", e => {
  e##preventDefault();
  log(Js.Array.map(t => t##identifier, Js.Array.from(e##targetTouches)));
  switch (touching^) {
  | First
  | Up(_) => ()
  | Down(startTime) =>
    let ellapsedTime = Js.Date.now() -. startTime;
    if (ellapsedTime < maxTimeForCountingSomethingAsTouch) {
      log("commit drawing");
      /* turn dots into an object */
      previousDrawing := None;
      log(strokes);
      strokes := Js.Dict.empty();
    } else {
      ();
        /* nothing happening here yet */
    };
  };
  touching := Up(Js.Date.now());
});

canvas##addEventListener("touchmove", e => {
  e##preventDefault();
  let lineWidth = 2;
  context##fillStyle #= foregroundColor;
  context##strokeStyle #= foregroundColor;
  context##lineWidth #= lineWidth;
  context##globalAlpha #= 1;

  Js.Array.forEachi(
    (touch, i) => {
      let point = {x: touch##clientX, y: touch##clientY};
      let id = Js.String.make(e##targetTouches[i]##identifier);
      let newPoints =
        switch (Js.Dict.get(strokes^, id)) {
        | None =>
          context##fillRect(point.x, point.y, lineWidth, lineWidth);
          [|point|];
        | Some([||]) => raise(Invalid_argument("impossible"))
        | Some(points) =>
          let prevPoint = points[Js.Array.length(points) - 1];
          context##beginPath();
          context##moveTo(prevPoint.x, prevPoint.y);
          context##lineTo(point.x, point.y);
          context##stroke();
          context##closePath();
          Js.Array.push(point, points)->ignore;
          points;
        };
      Js.Dict.set(strokes^, id, newPoints);
    },
    Js.Array.from(e##touches),
  );

  previousDrawing := Some("hipattern");
});
