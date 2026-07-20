import buildConfig from "../src/build.json" with { type: "json" };

const start = performance.now();
const result = await Bun.build({
  entrypoints: ["./src/assets/js/app.js"],
  outdir: "./dist/app/assets",
  naming: "app.min.js",
  define: {
    USE_FAKE_SOCKET: buildConfig.useFakeSocket ? "true" : "false",
  },
  minify: true,
  metafile: true,
});
const elapsed = performance.now() - start;

const inputs = Object.keys(result.metafile.inputs);
console.log(
  `Bundled ${inputs.length} modules in ${Math.round(elapsed)}ms with USE_FAKE_SOCKET=${buildConfig.useFakeSocket}`,
);

const sizeKB = (result.outputs[0].size / 1024).toFixed(2);
console.log(`\n  app.min.js  ${sizeKB} KB\n`);
