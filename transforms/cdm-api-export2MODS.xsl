<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="2.0" extension-element-prefixes="saxon"
    xmlns:saxon="http://saxon.sf.net/" xmlns:xs="http://www.w3.org/2001/XMLSchema"
    xmlns:mods="http://www.loc.gov/mods/v3" xmlns:cdm="http://oclc.org/cdm"
    xmlns:xlink="http://www.w3.org/1999/xlink">

    <!-- xsl to: 
        Convert ContentDM xml export to MODS per NYHS_ContentDM-to-Islandora_Mappings mapping 
    -->
    
    <!-- going to have to pass a parameter to set collection being processed. Nothing to go on in api output. 
         values: CivilWarTreasures, AmericanManuscripts, PhotosOfNYC, NYHSQuarterly, SlaveryManuscripts 
    -->
    
    <!-- 2015-09-15:
         * improved handling of publisher, distinguishing publisher of original and digitl
         * moved institution into /mods/local/physicalLocation
    -->
    <xsl:param name="current-collection" as="xs:string"/>
        
    <xsl:output method="xml" encoding="utf-8" indent="yes"/>

    <xsl:template match="/xml" exclude-result-prefixes="#all">
        <!-- this transform will produce individual records, not a collection, so mods is root element -->
        <mods xmlns="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="http://www.loc.gov/mods/v3 http://www.loc.gov/standards/mods/v3/mods-3-5.xsd"
            xmlns:xlink="http://www.w3.org/1999/xlink">
            <!-- can't be sure about multiple element occurences in general, best to plan ahead and miss nothing -->
            <!-- also, CDM puts in elements whether a value or not, so always test if empty first -->
            
            
            <!-- titles -->
            <xsl:for-each select="title, altern[. != '']">
                <titleInfo>
                    <xsl:if test="self::altern"><xsl:attribute name="type" select="string('alternative')"/></xsl:if>
                    <title><xsl:apply-templates/></title>
                </titleInfo>
            </xsl:for-each>
            
            <!-- parts -->
            <xsl:if test="(volume|issue)[. != '']">
                <part>
                    <detail type="volume"><number><xsl:apply-templates select="volume"/></number></detail>
                    <detail type="issue"><number><xsl:apply-templates select="issue"/></number></detail>
                </part>
            </xsl:if>
            
            <!-- identifier variations done by collection which is verbose but easiest and maintenance friendly -->
            <xsl:choose>
                <xsl:when test="$current-collection = 'AmericanManuscripts'">
                    <!-- uses identi for object filename, item for item ID -->
                    <xsl:if test="identi[. != '']">
                        <identifier type="local" displayLabel="NYHS filename"><xsl:apply-templates select="identi"/></identifier>
                    </xsl:if>
                    <xsl:if test="item[. != '']">
                        <identifier type="local" displayLabel="NYHS Item ID"><xsl:apply-templates select="item"/></identifier>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="$current-collection = 'CivilWarTreasures'">
                    <!-- object is used in CivilWarTreasures to store filename, identi for Item ID <sigh/>  -->
                    <xsl:if test="object[. != '']">
                        <identifier type="NYHS filename"><xsl:apply-templates select="object"/></identifier>
                    </xsl:if>
                    <xsl:if test="identi[. != '']">
                        <identifier type="local" displayLabel="NYHS Item ID"><xsl:apply-templates select="identi"/></identifier>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="$current-collection = 'PhotosOfNYC'">
                    <!-- <format> is used in PhotosOfNYC to store filename ... yikes, item for item ID -->
                    <xsl:if test="format[. != '']">
                        <identifier type="NYHS filename"><xsl:apply-templates select="format"/></identifier>
                    </xsl:if>
                    <xsl:if test="item[. != '']">
                        <identifier type="local" displayLabel="NYHS Item ID"><xsl:apply-templates select="item"/></identifier>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="$current-collection = 'SlaveryManuscripts'">
                    <!-- Slavery Manuscripts uses fileb for filename, item for item ID -->
                    <xsl:if test="fileb[. != '']">
                        <identifier type="NYHS filename"><xsl:apply-templates select="fileb"/></identifier>
                    </xsl:if>
                    <xsl:if test="item[. != '']">
                        <identifier type="local" displayLabel="NYHS Item ID"><xsl:apply-templates select="item"/></identifier>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="$current-collection = 'NYHSQuarterly'">
                    <!-- lccn numbers exist in NYHSQuarterly (only) -->
                    <xsl:if test="lccn[. != '']">
                        <identifier type="lccn" displayLabel="LCCN"><xsl:apply-templates select="lccn"/></identifier>
                    </xsl:if>
                    <xsl:if test="item[. != '']">
                        <identifier type="local" displayLabel="NYHS Item ID"><xsl:apply-templates select="item"/></identifier>
                    </xsl:if>
                </xsl:when>
            </xsl:choose>
            <!-- all collections consistently use dmrecord -->
            <xsl:for-each select="dmrecord[. != '']">
                <identifier type="local" displayLabel="ContentDM"><xsl:apply-templates select="."/></identifier>
            </xsl:for-each>
                        
            <!-- creator(s) -->
            <xsl:if test="creato[. != '']">
                <xsl:for-each select="tokenize(creato,';')">
                    <name>
                        <namePart><xsl:value-of select="normalize-space(.)"/></namePart>
                        <role><roleTerm type="code" authority="marcrelator">cre</roleTerm></role>
                        <role><roleTerm type="text" authority="marcrelator">creator</roleTerm></role>
                    </name>
                </xsl:for-each>
            </xsl:if>

            <!-- contributor(s) -->
            <xsl:if test="contri[. != '']">
                <xsl:for-each select="tokenize(contri,';')">
                    <name>
                        <namePart><xsl:value-of select="normalize-space(.)"/></namePart>
                        <role><roleTerm type="code" authority="marcrelator">ctb</roleTerm></role>
                        <role><roleTerm type="text" authority="marcrelator">contributor</roleTerm></role>
                    </name>
                </xsl:for-each>
            </xsl:if>
            
            <!-- abstract -->
            <abstract>
                <xsl:apply-templates select="descri"/>
            </abstract>

            <!-- language -->
            <xsl:if test="langua">
                <language>
                    <languageTerm authority="iso639-2b" type="code"><xsl:apply-templates select="langua"/></languageTerm>
                </language>
            </xsl:if>
            
            <!-- CivilWarTreasures, AmericanManuscripts, PhotosOfNYC, NYHSQuarterly, SlaveryManuscripts  -->
            <!-- Physical description variations done by collection which is verbose but easiest and maintenance friendly -->
            <xsl:if test="(medium|file|format|filea)[. != '']">
                <physicalDescription>
                    <!-- use of medium is consistent across collections that use it -->
                    <xsl:if test="medium">
                        <form><xsl:apply-templates select="medium"/></form>
                    </xsl:if>
                    <xsl:choose>
                        <xsl:when test="$current-collection = 'AmericanManuscripts'">
                            <!-- uses file for filesize -->
                            <xsl:if test="file[. != '']">
                                <extent><xsl:apply-templates select="file"/></extent>
                            </xsl:if>
                        </xsl:when>
                        <xsl:when test="$current-collection = 'CivilWarTreasures'">
                            <!-- uses file for filesize, format for form -->
                            <xsl:if test="format[. != '']">
                                <form><xsl:apply-templates select="format"/></form>
                            </xsl:if>
                            <xsl:if test="file[. != '']">
                                <extent><xsl:apply-templates select="file"/></extent>
                            </xsl:if>
                        </xsl:when>
                        <xsl:when test="$current-collection = 'PhotosOfNYC'">
                            <!-- file for extent -->
                            <xsl:if test="file[. != '']">
                                <extent><xsl:apply-templates select="file"/></extent>
                            </xsl:if>
                        </xsl:when>
                        <xsl:when test="$current-collection = 'SlaveryManuscripts'">
                            <!-- filea for extent, format for form -->
                            <xsl:if test="filea[. != '']">
                                <extent><xsl:apply-templates select="filea"/></extent>
                            </xsl:if>
                            <xsl:if test="format[. != '']">
                                <form><xsl:apply-templates select="format"/></form>
                            </xsl:if>
                        </xsl:when>
                        <xsl:when test="$current-collection = 'NYHSQuarterly'">
                            <xsl:if test="format[. != '']">
                                <form><xsl:apply-templates select="format"/></form>
                            </xsl:if>
                        </xsl:when>
                    </xsl:choose>
                </physicalDescription>
            </xsl:if>
            
            <!-- type -->
            <!-- <source> used only by NYHSQuarterly for this, doesn't exist in other collections 
                 so no specific logic required -->
            <xsl:if test="type | dcmi | source">
                <xsl:for-each select="type | dcmi | materi">
                    <xsl:for-each select="tokenize(.,';')">
                        <genre><xsl:value-of select="normalize-space(.)"/></genre>
                    </xsl:for-each>
                </xsl:for-each>
            </xsl:if>
            
            <!-- subjects... don't forget to reuse date for temporal :) -->
            <subject>
                <!-- subjects -->
                <xsl:if test="subjec">
                    <xsl:for-each select="tokenize(subjec,';')">
                        <topic><xsl:value-of select="normalize-space(.)"/></topic>
                    </xsl:for-each>
                </xsl:if>
                <!-- date -->
                <xsl:if test="date">
                    <!-- turns out in rare cases there can be more than one date so use for-each -->
                    <xsl:for-each select="date">
                        <xsl:for-each select="tokenize(.,';')">
                            <temporal><xsl:value-of select="normalize-space(.)"/></temporal>
                        </xsl:for-each>
                    </xsl:for-each>
                </xsl:if>
                <!-- coverage (geo), only in NYHS Quarterly -->
                <xsl:if test="covera">
                    <xsl:for-each select="tokenize(covera,';')">
                        <geographic><xsl:value-of select="normalize-space(.)"/></geographic>
                    </xsl:for-each>
                </xsl:if>
            </subject>
            
            <!-- record info, applies to generation of MODS record -->
            <recordInfo>
                <recordCreationDate><xsl:value-of select="current-date()"/></recordCreationDate>
                <recordOrigin>Created programmatically from ContentDM export.</recordOrigin>
            </recordInfo>
            
            <!-- rights -->
            <xsl:if test="rights">
                <accessCondition><xsl:apply-templates select="rights"/></accessCondition>
            </xsl:if>

            <!-- origin info (provenance) -->
            <originInfo eventType="digitalPublication">
                <xsl:if test="dmcreated[. != '']">
                       <dateCreated><xsl:apply-templates select="dmcreated"/></dateCreated>
                </xsl:if>
                <xsl:if test="dmmodified[.  != '']">
                    <dateModified><xsl:apply-templates select="dmmodified"/></dateModified>
                </xsl:if>
                <!-- date of digital (ization?) -->
                <xsl:if test="(datea|datei)[. != '']">
                    <dateCaptured><xsl:apply-templates select="datea"/></dateCaptured>
                </xsl:if>
                <!-- for now publisher of digital here, publisher of original in another originInfo element -->
                <xsl:if test="publia[. != '']">
                    <xsl:for-each select="publia[. != '']">
                        <publisher><xsl:apply-templates select="."/></publisher>
                    </xsl:for-each>
                </xsl:if>
            </originInfo>

            <xsl:if test="publis[. != '']">
                <xsl:for-each select="publis[. != '']">
                    <originInfo eventType="originalPublication">
                        <publisher><xsl:apply-templates select="."/></publisher>
                    </originInfo>
                </xsl:for-each>
            </xsl:if>
            
            

            <xsl:if test="instit[. != '']">
                <xsl:for-each select="instit[. != '']">
                    <location><physicalLocation><xsl:apply-templates select="."/></physicalLocation></location>
                </xsl:for-each>
            </xsl:if>
            

            <!-- notes -->
            <xsl:for-each select="notes[. != '']">
                <note><xsl:apply-templates select="."/></note>
            </xsl:for-each>
            
            
            <!-- put in related items for collection, series, and subseries titles . . . and plain ole related items 
                 unfortunately different conventions across collections for the collection id, title, and url  -->
            <xsl:choose>
                <!-- CivilWarTreasures has a different mapping of collection title, collection url -->
                <xsl:when test="$current-collection = 'CivilWarTreasures'">
                    <xsl:for-each select="relatig[. != ''], series[. != '']">
                        <xsl:variable name="relationship-type">
                            <xsl:choose>
                                <xsl:when test="self::relatig"><xsl:text>host</xsl:text></xsl:when>
                                <xsl:when test="self::series"><xsl:text>series</xsl:text></xsl:when>
                            </xsl:choose>
                        </xsl:variable>
                        <xsl:variable name="display-label">
                            <xsl:choose>
                                <xsl:when test="self::relatig"><xsl:text>Collection</xsl:text></xsl:when>
                                <xsl:when test="self::series"><xsl:text>Series</xsl:text></xsl:when>
                            </xsl:choose>
                        </xsl:variable>
                        <relatedItem type="{$relationship-type}" displayLabel="{$display-label}">
                            <titleInfo><title><xsl:value-of select="normalize-space(.)"/></title></titleInfo>
                            <!-- if this is a collection add the id and, if exists, collection url in -->
                            <xsl:if test="self::relatig">
                                <identifier type="local"><xsl:value-of select="preceding-sibling::collec"/></identifier>
                            </xsl:if>
                            <xsl:if test="self::relatig and parent::xml/collea[. != '']">
                                    <location><url><xsl:apply-templates select="parent::xml/collea"/></url></location>
                            </xsl:if>
                            <!-- subseries handled in a nested relatedItem -->
                            <xsl:if test="self::series and parent::xml/subser[. != '']">
                                <relatedItem type="series" displayLabel="Subseries">
                                    <titleInfo><title><xsl:apply-templates select="parent::xml/subser[. != '']"/></title></titleInfo>
                                </relatedItem>
                            </xsl:if>
                        </relatedItem>
                    </xsl:for-each>              
                </xsl:when>
                <xsl:otherwise>
                    <xsl:for-each select="(collea|relatig)[. != ''], series[. != ''], relati[. != '']">
                        <xsl:variable name="relationship-type">
                            <xsl:choose>
                                <xsl:when test="matches(name(),'(collea|relatig)')"><xsl:text>host</xsl:text></xsl:when>
                                <xsl:when test="self::series"><xsl:text>series</xsl:text></xsl:when>
                                <xsl:when test="self::relati"><xsl:text>references</xsl:text></xsl:when>
                            </xsl:choose>
                        </xsl:variable>
                        <xsl:variable name="display-label">
                            <xsl:choose>
                                <xsl:when test="matches(name(),'(collea|relatig)')"><xsl:text>Collection</xsl:text></xsl:when>
                                <xsl:when test="self::series"><xsl:text>Series</xsl:text></xsl:when>
                                <!-- display label for <relati> purposefully omitted, we don't know what it is -->
                            </xsl:choose>
                        </xsl:variable>
                        <relatedItem type="{$relationship-type}" displayLabel="{$display-label}">
                            <titleInfo><title><xsl:value-of select="normalize-space(.)"/></title></titleInfo>
                            <!-- if this is a collection add the id and, if exists, collection url in -->
                            <xsl:if test="name() = 'collea'">
                                <identifier type="local"><xsl:value-of select="preceding-sibling::collec"/></identifier>
                            </xsl:if>
                            <xsl:if test="self::collea and parent::xml/colleb[. != '']">
                                    <location><url><xsl:apply-templates select="parent::xml/colleb"/></url></location>
                            </xsl:if>
                            <!-- subseries handled in a nested relatedItem -->
                            <xsl:if test="self::series and parent::xml/subser[. != '']">
                                <relatedItem type="series" displayLabel="Subseries">
                                    <titleInfo><title><xsl:apply-templates select="parent::xml/subser[. != '']"/></title></titleInfo>
                                </relatedItem>
                            </xsl:if>
                        </relatedItem>
                    </xsl:for-each>
                </xsl:otherwise>
            </xsl:choose>
            
            
            
            
            
            <!-- for now dropping full text fields into extension until answer on how to output for migration -->
            <extension>
                <xsl:if test="full[. != '']">
                    <cdm:fulltext><xsl:apply-templates select="full"/></cdm:fulltext>
                </xsl:if>
                <xsl:if test="$current-collection = 'AmericanManuscripts' and format[. != '']">
                    <!-- format used in this collection to store full text ... yikes -->
                    <cdm:fulltext><xsl:apply-templates select="format"/></cdm:fulltext>
                </xsl:if>
                <xsl:if test="$current-collection = 'NYHSQuarterly' and transc[. != '']">
                    <!-- transcr used in this collection to store full text -->
                    <cdm:fulltext><xsl:apply-templates select="transc"/></cdm:fulltext>
                </xsl:if>
            </extension>
            
        </mods>
    </xsl:template>
    
    <xsl:template match="text()">
        <xsl:value-of select="normalize-space(replace(.,'&lt;br&gt;',', '))"/>
    </xsl:template>

</xsl:stylesheet>
