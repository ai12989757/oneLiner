while(size(`ls "pasted__*"`)>0) searchReplaceNames "pasted__" " " "all";

while len(ls('pasted__*')) > 0:
    delete('pasted__*')