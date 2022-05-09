import express from "express";
import { json } from "body-parser";

const app = express();
const port = 3000;

app.use(json());

app.post("/sensor/temp", (req: any, res: any) => {
  console.log(req.body);
  res.send(req.body);
});

app.listen(port, () => {
  console.log(`Listening on port ${port}`);
});
