/* eslint-env node */
require("@rushstack/eslint-patch/modern-module-resolution");

module.exports = {
  root: true,
  extends: ["eslint:recommended", "prettier"],
  parserOptions: {
    sourceType: "module",
  },
  env: {
    node: true,
    es2022: true,
  },
};
