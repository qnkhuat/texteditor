Following this tutorial:[link](https://viewsourcecode.org/snaptoken/kilo/) + [Source code](https://github.com/snaptoken/kilo-src)

It's very well written. Check it out.

In this repo I added some minors features:
- Toggle between insert/visual mode like vim
- Navigate with : WASD, HJKL.
- Display line number
- Jump to line number

Original features:
- Display text file
- Navigate in files with arrows
- Search
- Text highlight

To run: 
- build: `make`
- openup: `./voila FILE_PATH`

### TODO:
- [x] insert/visual mode
- [x] Navigate with WASD and HJKL
- [x] Add display line numbers
- [x] Jump to line
- [ ] Refine display line features, need to dynamically allocate number size

### Use it everywhere
If you  desire to use `voila` everywhere. Run this command in terminal: `echo "alias voila=$(pwd)/voila" >> ~/.bashrc`

Now  you can open voila with any file anywhere. `voila FILE_PATH`

Though I don't recommend you use it because it's a poor and just-for-fun editor lol. Instead use vim / emacs.

I tried to open an 1GB text file and vim open it up in about 5 seconds and edit without any problems.

Voila instead take ~90 seconds to load the file, but editing works just fine since the program only process lines that are  in view.

Of course huh, How does this editor can compare to legend like vim =))🔥
