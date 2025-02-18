# Versioned documentation deployment

- Assumed to be deployed via eos using a folder structure like this
```console
.
├── _contents
│   ├── vXX-YY
│   └── master
├── index.html
```
  - Each version / tag gets a new folder and the `master` / develop version is
    continually updated
  - The `_contents` folder has to configured to have *Directory Browsing* enabled
    since that will be used to determine the available versions (each top-level
    folder in here will be considered a version). This is achieved via `echo
    "Options +Indexes > _contents/.htaccess"`
- The `version.html` "template" fetches the directory from `_contents`, extracts
  the available directories (via their links) and creates a sorted list of
  versions before dynamically populating the read-the-docs bit for version
  selection
- gitlab ci takes care of copying the generated contents to the correct
  subfolder of `_contents` depending on whether it has been triggered by a tag
  or by a push to master
- Documentation for versions that are added post-hoc need a small adaption of
  their (sphinx generated) `index.html`; the `version.html` template has to be
  copied verbatim to the bottom, but above the navigation enabling.
- The `index.html` in this folder has to be placed into the same folder where
  also `_contents` is
