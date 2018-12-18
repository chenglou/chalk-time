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

type point = {
  x: int,
  y: int,
};

[@bs.module]
external pointInPolygon: (point, array(point)) => bool = "point-in-polygon";

let highlightPoints = (~color="red", points) => {
  let nodesContainer = document##createElement("div");

  Belt.Array.forEach(
    points,
    ({x, y}) => {
      let node = document##createElement("div");
      node##style##width #= "5px";
      node##style##height #= "5px";
      node##style##border #= {j|1px solid $color|j};
      node##style##position #= "absolute";
      node##style##borderRadius #= "9px";
      node##style##top #= {j|$(y)px|j};
      node##style##left #= {j|$(x)px|j};
      node##style##zIndex #= "99";
      nodesContainer##appendChild(node);
    },
  );
  document##body##appendChild(nodesContainer);
  Js.Global.setTimeout(
    () => document##body##removeChild(nodesContainer),
    1000,
  )
  ->ignore;
};

let (width, height) = (window##innerWidth, window##innerHeight);

let context = {
  let canvas = document##createElement("canvas");
  canvas##width #= width;
  canvas##height #= height;
  canvas##style##position #= "absolute";
  document##body##appendChild(canvas);

  let context = canvas##getContext("2d");
  context##fillStyle #= backgroundColor;
  context##fillRect(0, 0, width, height);
  context;
};

/* next: implement commit workflow #2
   - selectively commit (pixie dust) */

type touching =
  | Down(float)
  | Up(float)
  | First;
let touching = ref(First);

let strokes: ref(Js.Dict.t(array(point))) = ref(Js.Dict.empty());
let objectsOnScene = [||];
let objectTypes = [|Objs.obj1|];
/* TODO: Objs.obj1 has too many points */

context##canvas##addEventListener("touchstart", e => {
  e##preventDefault();

  touching := Down(Js.Date.now());
});

let distance = (p1, p2) => {
  let x = p1.x - p2.x;
  let y = p1.y - p2.y;
  Js.Math.sqrt(float_of_int(x * x + y * y));
};

context##canvas##addEventListener("touchend", e => {
  e##preventDefault();

  switch (touching^) {
  | First
  | Up(_) => ()
  | Down(startTime) =>
    let liftedTouches =
      Js.Array.map(
        touch => Js.String.make(touch##identifier),
        Js.Array.from(e##changedTouches),
      );

    let liftedStrokes =
      Belt.Array.keepMap(
        liftedTouches,
        id => {
          let stroke = Js.Dict.get(strokes^, id);
          switch (stroke) {
          | None => None
          | Some(stroke) => Some(stroke)
          };
        },
      );

    let otherStrokes =
      Belt.Array.keepMap(
        /* [(id1, [...]), (id2, [...])] */

          Js.Dict.entries(strokes^),
          ((id, stroke)) => {
            let isLiftedStroke =
              Js.Array.find(touchId => touchId == id, liftedTouches);
            switch (isLiftedStroke) {
            | None => Some(stroke)
            | Some(_) => None
            };
          },
        );

    Belt.Array.forEach(
      liftedStrokes,
      liftedStroke => {
        highlightPoints(~color="green", liftedStroke);

        let asd =
          Js.Array.forEach(
            otherStroke => {
              let otherStrokeHasSomePointsInLiftedStrokeBounds =
                Js.Array.some(
                  point => pointInPolygon(point, liftedStroke),
                  otherStroke,
                );
              if (otherStrokeHasSomePointsInLiftedStrokeBounds) {
                highlightPoints(otherStroke);
              };
            },
            otherStrokes,
          );
        ();
      },
    );

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
        context##lineWidth #= lineWidth;
        Js.Array.forEach(
          points =>
            Js.Array.forEachi(
              (point, i) =>
                if (i == 0) {
                  context##fillRect(
                    point.x + 10,
                    point.y + 10,
                    lineWidth,
                    lineWidth,
                  );
                } else {
                  let prevPoint = points[i - 1];
                  context##beginPath();
                  context##moveTo(prevPoint.x + 10, prevPoint.y + 10);
                  context##lineTo(point.x + 10, point.y + 10);
                  context##stroke();
                  context##closePath();
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
});

context##canvas##addEventListener("touchmove", e => {
  e##preventDefault();

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
});
