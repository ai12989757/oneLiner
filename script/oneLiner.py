#!/usr/bin/python
# -*- coding: utf-8 -*-
#---------written by:----------------------
#-------Fauzan Syabana---------------------
#------zansyabana@gmail.com----------------
#Licensed under MIT License
#------------------------------------------
#----------change by:----------------------
#------------yibai-------------------------
#--------159689757@qq.com------------------
# Change Log: 
# - support python 3.x
# - create new UI
#------------------------------------------

from pymel.core import *

#selector
def selector(lookName):
    nameSplit = lookName.split(':')
    method = nameSplit[0]
    nameSel = nameSplit[1]
    if method == 'f':
        sel = ls("*{}*".format(nameSel), r=True)
    elif method == 'fe':
        sel = ls("*{}".format(nameSel), r=True)
    elif method == 'fs':
        sel = ls("{}*".format(nameSel), r=True)
    select(sel)

def oneLiner(nName, method='s'):

    # get selection method
    if nName.find(':') != -1:
        selector(nName)
    elif nName.find('/s') != -1:
        method = 's'
        nName = nName.replace('/s', '')
    elif nName.find('/h') != -1:
        method = 'h'
        nName = nName.replace('/h', '')
    elif nName.find('/a') != -1:
        if nName.find('>') != -1:
            method = 'a'
            print(method)
        nName = nName.replace('/a', '')

    if method == 's':
        slt = selected()
    elif method == 'h':
        sltH = []
        slt = selected()
        for i in slt:
            sltH.append(i)
            for child in reversed(i.listRelatives(ad=True, type='transform')):
                sltH.append(child)
        print(sltH)
        slt = sltH
    elif method == 'a':
        slt = ls()

    # find numbering replacement
    def numReplace(numName, idx, start=1):
        global padding
        alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        if numName.find('//') != -1:
            start = int(numName[numName.find('//') + 2:len(numName)])
            numName = numName.replace(numName[numName.find('//'):len(numName)], '')
            print(start)
        number = idx + start
        if numName.find('#') != -1:
            padding = numName.count('#')
            hastag = "{0:#>{1}}".format("#", padding)  # get how many '#' is in the new name
            num = "{0:0>{1}d}".format(number, padding)  # get number
            numName = numName.replace(hastag, num)
        # alphabetical numbering
        if numName.find('@') != -1:
            padding = numName.count('@')
            aSym = "{0:@>{1}}".format("@", padding) # get how many '@' is in the new name
            alphaNum = "{}".format(alpha[idx])  # get alphabetical number
            numName = numName.replace(aSym, alphaNum)

        return numName
    if nName.find(':') == -1:
        for i in slt:  # for every object in selection list
            # check if there is '>' that represents the replacement method
            if i.name().find('|') !=-1:
                curName = i.name().split('|')[-1]
            else:
                curName = i.name()
            if nName.find('>') != -1:
                wordSplit = nName.split('>')
                oldWord = wordSplit[0]
                newWord = numReplace(wordSplit[1],slt.index(i))
                try:
                    rename(i,i.replace(oldWord,newWord))
                except:
                    print("{} is not renamed".format(i))

            # check if the first character is '-' or '+', remove character method
            elif nName[0] == '-':
                charToRemove = int(nName[1:len(nName)])
                try:
                    rename(i, i[0:-charToRemove])
                except:
                    print("{} is not renamed".format(i))

            elif nName[0] == '+':
                charToRemove = int(nName[1:len(nName)])
                rename(i, i[charToRemove:len(curName)])

            else:
                newName = numReplace(nName, slt.index(i))
                # get current Name if '!' mentioned
                print(i)
                if newName.find('!') != -1:
                    newName = newName.replace('!', curName)
                    print(newName)
                try:
                    rename(i, newName)
                except:
                    print("{} is not renamed".format(i))

def newNameView(nName, method='s'):
    changeName = []
    # get selection method
    if nName.find(':') != -1:
        pass
        #selector(nName)
    elif nName.find('/s') != -1:
        method = 's'
        nName = nName.replace('/s', '')
    elif nName.find('/h') != -1:
        method = 'h'
        nName = nName.replace('/h', '')
    elif nName.find('/a') != -1:
        if nName.find('>') != -1:
            method = 'a'
            #print(method)
        nName = nName.replace('/a', '')

    if method == 's':
        slt = selected()
    elif method == 'h':
        sltH = []
        slt = selected()
        for i in slt:
            sltH.append(i)
            for child in reversed(i.listRelatives(ad=True, type='transform')):
                sltH.append(child)
        #print(sltH)
        slt = sltH
    elif method == 'a':
        slt = ls()

    # find numbering replacement
    def numReplace(numName, idx, start=1):
        global padding
        alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        if numName.find('//') != -1:
            start = int(numName[numName.find('//') + 2:len(numName)])
            numName = numName.replace(numName[numName.find('//'):len(numName)], '')
            #print(start)
        number = idx + start
        if numName.find('#') != -1:
            padding = numName.count('#')
            hastag = "{0:#>{1}}".format("#", padding)  # get how many '#' is in the new name
            num = "{0:0>{1}d}".format(number, padding)  # get number
            numName = numName.replace(hastag, num)
        # alphabetical numbering
        if numName.find('@') != -1:
            padding = numName.count('@')
            aSym = "{0:@>{1}}".format("@", padding) # get how many '@' is in the new name
            alphaNum = "{}".format(alpha[idx])  # get alphabetical number
            numName = numName.replace(aSym, alphaNum)

        return numName
    if nName.find(':') == -1:
        for i in slt:  # for every object in selection list
            # check if there is '>' that represents the replacement method
            if i.name().find('|') !=-1:
                curName = i.name().split('|')[-1]
            else:
                curName = i.name()
                
            if nName.find('>') != -1:
                wordSplit = nName.split('>')
                oldWord = wordSplit[0]
                newWord = numReplace(wordSplit[1],slt.index(i))
                newName = i.replace(oldWord,newWord)
                changeName.append(newName)
            # check if the first character is '-' or '+', remove character method
            elif nName[0] == '-':
                charToRemove = int(nName[1:len(nName)])
                newName = i[0:-charToRemove]
                changeName.append(newName)
            elif nName[0] == '+':
                charToRemove = int(nName[1:len(nName)])
                newName = i[charToRemove:len(curName)]
                changeName.append(newName)
            else:
                newName = numReplace(nName, slt.index(i))
                # get current Name if '!' mentioned
                #print(i)
                if newName.find('!') != -1:
                    newName = newName.replace('!', curName)
                changeName.append(newName)
        
    return changeName
"""
### UI

class oneLinerWindow(object):

    windowName = "OneLiner"

    def show(self):

        if window(self.windowName, q=True, exists=True):
            deleteUI(self.windowName)
            windowPref(self.windowName, remove=True)
        window(self.windowName, s=True, w=300, h=10, rtf=False, tb=True)

        self.buildUI()

        showWindow()

    def buildUI(self):
        toolTip = 'Character replacement symbols:' \
                  '\n! = old name' \
                  '\n# = numbering based on selection, add more # for more digits' \
                  '\n@ = alphabetical numbering based on selection' \
                  '\n\nFind and replace method:' \
                  '\n"oldName">"newName" (without quotes)' \
                  '\n\nRemove first or last character(s):' \
                  '\n-(amount of characters to remove) = removes specific amounts of characters from last character'\
                  '\n+(amount of characters to remove) = removes specific amounts of characters from first character' \
                  '\n\nAdd these symbols at the end to change the options:' \
                  '\n//(number) = define the start number of numbering from #' \
                  '\n/s = selected only (this is default, you dont have to type this)' \
                  '\n/h = add items from all hierarchy descendants of selected items' \
                  '\n\nAdditional tool:'\
                  '\nadd f: at the start of the text to find objects within desired characters'\
                  '\nadd fe: at the start of the text to find objects that ends with the desired characters'\
                  '\nadd fs: at the start of the text to find objects the starts with the desired scharacters'

        column = columnLayout(cal='center', adj=True)
        separator(h=1, style='none')
        self.rnmInput = textField(ec=self.runFunc, aie=True, w=200, ann=toolTip)
        separator(h=1, style='none')

    def runFunc(self,*args):
        self.rnmQ = textField(self.rnmInput, text=True,q=True)
        oneLiner(self.rnmQ)

oneLinerWindow().show()
"""