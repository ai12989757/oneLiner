# OneLiner

Simple Renamer Tool for Maya

# Language

[English](README.md)/[简体中文](RREADME_zh.md)

## UI

![image.png](images/10.png)

## ENV

- windows
- python3.x
- PySide2
- maya2019-2023
- pymel-1.2.0

## Using tutorials

### Instll

- Download and unzip the oneLiner.zip
- Drag and drop the "studiolibrary/install.mel" file onto the Maya viewport
- Click the Studio Library icon on the shelf to run

    **tips：It is recommended to set it as a shortcut key**

### Details

#### Simple

- select object run the script,Enter text

![01.gif](images/01.gif)

##### > Find and replace

- "oldName">"newName" (without quotes)

![03.gif](images/03.gif)

##### USE ! # @

- **!** old name

![02.gif](images/02.gif)

- **#** numbering based on selection, add more # for more digits

![01.gif](images/01.gif)

- **@** alphabetical numbering based on selection

![04.gif](images/04.gif)

##### /s /h selected/hierarchy

- **/s** selected only (this is default, you dont have to type this)
- **/h** add items from all hierarchy descendants of selected items

![05.gif](images/05.gif)

##### + - -- emove first or last character(s)

- **+num** +(amount of characters to remove) = removes specific amounts of characters from first character
- **-num** -(amount of characters to remove) = removes specific amounts of characters from last character
- **--num** Delete to leave only

![06.gif](images/06.gif)

##### f: fs: fe: 附加功能

- **f: xxx** at the start of the text to find objects within desired characters
- **fs: xxx** at the start of the text to find objects that ends with the desired characters
- **fe: xxx** at the start of the text to find objects the starts with the desired scharacters

![07.gif](images/07.gif)

##### Right click Show Tips

## Thanks

- **Original Author:**

    Fauzan Syabana

    zansyabana@gmail.com

    rename code for Fauzan Syabana

- **oneLiner:**

    [https://www.highend3d.com/maya/script/oneliner-simple-renamer-tool-for-maya](<https://www.highend3d.com/maya/script/oneliner-simple-renamer-tool-for-maya>)

    ![09.png](images/09.png)
