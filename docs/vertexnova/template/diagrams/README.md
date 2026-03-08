# Diagrams

Draw.io source files for the VneTemplate documentation.

## Export to PNG

The `template.md` document references PNG images. Export the `.drawio` files to PNG using one of these methods:

### Option 1: draw.io Desktop (macOS/Windows/Linux)

If [draw.io Desktop](https://github.com/jgraph/drawio-desktop/releases) is installed:

```bash
drawio -x -f png -o context.png context.drawio
drawio -x -f png -o api.png api.drawio
drawio -x -f png -o architecture.png architecture.drawio
```

Or export all at once:

```bash
drawio -x -f png -o . .
```

### Option 2: draw.io Web

1. Open [app.diagrams.net](https://app.diagrams.net)
2. File → Open from → Device → select each `.drawio` file
3. File → Export as → PNG
4. Save to this `diagrams/` folder

### Files

| Source | Output | Used in template.md | Contents |
|--------|--------|---------------------|----------|
| context.drawio | context.png | Overview | C4 system context: Application → VneTemplate (get_version, hello) |
| api.drawio | api.png | API usage | API flow: get_version(), hello() |
| architecture.drawio | architecture.png | Project layout | Folder structure and build flow |
