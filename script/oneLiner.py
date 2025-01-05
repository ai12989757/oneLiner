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
# - fix f:/fs:/fe: can't select all
# - - + don't rename shape name
# - fix - + can't rename shadingEngine node
#------------------------------------------
# 修复有重名对象时的问题
# 优化/h模式下的选择，会排除掉 shape 下如果父级在 selection 中的对象

import maya.cmds as cmds

# region 撤回块
# 定义一个装饰器，用来存放撤回块
def undoBlock(func):
    def wrapper(*args, **kwargs):
        # 开始撤回块
        cmds.undoInfo(openChunk=True)
        # 执行函数
        func(*args, **kwargs)
        # 结束撤回块
        cmds.undoInfo(closeChunk=True)
    return wrapper

#selector
def selector(lookName):
    nameSplit = lookName.split(':')
    method = nameSplit[0]
    nameSel = nameSplit[1]
    if method == 'f':
        sel = cmds.ls("*{}*".format(nameSel), r=True)
    elif method == 'fe':
        sel = cmds.ls("*{}".format(nameSel), r=True)
    elif method == 'fs':
        sel = cmds.ls("{}*".format(nameSel), r=True)
    Temp = []
    for child in sel:
        if cmds.nodeType(child) != 'transform' and cmds.nodeType(child) != 'joint':
            if cmds.listRelatives(child, p=True) != None:
                for i in cmds.listRelatives(child, p=True):
                    if i in sel:
                        Temp.append(child)
                        continue
    for i in Temp:
        sel.remove(i)
    cmds.select(sel, ne=True)

def getNweName(nName, method='s'):
    changeName = []
    if nName == '/':
        pass
    # get selection method
    if ':' in nName:
        selector(nName)
    elif '/s' in nName:
        method = 's'
        nName = nName.replace('/s', '')
    elif '/h' in nName:
        method = 'h'
        nName = nName.replace('/h', '')
    elif '/a' in nName:
        if '>' in nName:
            method = 'a'
            #print(method)
        nName = nName.replace('/a', '')

    if method == 's':
        slt = cmds.ls(selection=True)
    elif method == 'h':
        sltH = []
        slt = cmds.ls(selection=True)
        # 两两比较 slt，isParent 为 True 则去掉第二个
        if len(slt) > 1:
            sel = cmds.ls(selection=True, long=True)
            to_remove = set()
            for i in sel:
                for j in sel:
                    if i != j and isParent(i, j):
                        to_remove.add(j)

            # 从 slt 中移除需要删除的元素
            sel = [item for item in sel if item not in to_remove]
            slt = []
            for i in sel:
                if '|' in i:
                    if len(cmds.ls(i.split('|')[-1])) == 1:
                        i = i.split('|')[-1]
                slt.append(i)
        print(slt)

        for i in slt:
            sltH.append(i)
            relatives,fullname = cmds.listRelatives(i, ad=True, type='transform'), cmds.listRelatives(i, ad=True, fullPath=True, type='transform')
            relatives = list(reversed(relatives))
            fullname = list(reversed(fullname))
            if relatives:
                for index, child in enumerate(relatives):
                    if len(cmds.ls(child)) > 1:
                        child = fullname[index]
                    sltH.append(child)
        slt = sltH
    elif method == 'a':
        slt = cmds.ls()

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

    if not nName or nName == '/':
        return  slt, slt
    if ':' not in nName:
        for i in slt:  # for every object in selection list
            # check if there is '>' that represents the replacement method
            if '|' in i:
                curName = i.split('|')[-1]
            else:
                curName = i
            if '>' in nName:
                wordSplit = nName.split('>')
                oldWord = wordSplit[0]
                newWord = numReplace(wordSplit[1],slt.index(i))
                newName = i.replace(oldWord,newWord)
            # check if the first character is '-' or '+', remove character method
            elif nName[0] == '-':
                charToRemove = int(nName[1:len(nName)])
                newName = curName[0:-charToRemove]
            elif nName[0] == '+':
                charToRemove = int(nName[1:len(nName)])
                newName = curName[charToRemove:len(curName)]
            else:
                newName = numReplace(nName, slt.index(i))
                # get current Name if '!' mentioned
                #print(i)
                if newName.find('!') != -1:
                    newName = newName.replace('!', curName)
            if newName:
                if newName == '/':
                    return slt, slt
                if '|' in newName:
                    newName = newName.split('|')[-1]
                if len(cmds.ls(newName)) > 1:
                    newName = renameAddDigit(newName)
            changeName.append(newName)

        if any('|' in i for i in slt) or any('|' in i for i in changeName):
            slt.reverse()
            changeName.reverse()
    return slt, changeName

@undoBlock
def oneLiner(nName, method='s'):
    if not nName or nName == '/':
        return
    elif nName == '/h':
        cmds.select(getNweName(nName, method)[0], r=True)
        return
    slt, newNameList = getNweName(nName, method)
    for i, newName in zip(slt, newNameList):
        try:
            cmds.rename(i, newName)
        except:
            print("{} is not renamed".format(i))
        
def newNameView(nName, method='s'):
    return getNweName(nName, method)[1]
    

# 查询两个对象是否存在层级关系
def isParent(itemA, itemB):
    if not itemA or not itemB:
        return False
    if itemA == itemB:
        return True
    if cmds.listRelatives(itemA, ad=True):
        if itemB in cmds.listRelatives(itemA, ad=True, fullPath=True):
            return True
    return False

def renameAddDigit(name):
    if '|' in name:
        name = name.split('|')[-1]
    digit = 0
    if name[-1].isdigit():
        while name[-1].isdigit():
            digit = digit + int(name[-1])
            name = name[:-1]
        
    if cmds.objExists(name):
        while cmds.objExists(name + str(digit)):
            digit += 1
        name = name + str(digit + 1)
    else:
        name = name + str(digit)
    return name
    
@undoBlock
def renamePastedPrefix():
    index = 0
    while cmds.ls("pasted__*") != []:
        index += 1
        if index > 100:
            break
        for i in cmds.ls("pasted__*"):
            name = i.replace("pasted__", "")
            try:
                cmds.rename(i, renameAddDigit(name))
            except:
                pass
                    