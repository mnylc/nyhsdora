<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:mods="http://www.loc.gov/mods/v3"
                xmlns="http://www.loc.gov/mods/v3"
                exclude-result-prefixes="mods"
                version="1.0">
    <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes" media-type="text/xml"/>
    <xsl:strip-space elements="*"/>
    <xsl:template match="node()|@*">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
        </xsl:copy>
    </xsl:template>

    <xsl:template match="mods:originInfo/mods:dateCreated/text()">
        <xsl:value-of select="/mods:mods/mods:subject/mods:temporal"/>
    </xsl:template>
    <xsl:template match="mods:originInfo/mods:dateModified"/>
    <xsl:template match="mods:subject/mods:temporal"/>

    <xsl:template match="mods:recordInfo">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
        </xsl:copy>
        <recordInfo>
            <recordCreationDate>
                <xsl:value-of select="/mods:mods/mods:originInfo/mods:dateCreated"/>
            </recordCreationDate>
            <recordChangeDate>
                <xsl:value-of select="/mods:mods/mods:originInfo/mods:dateModified"/>
            </recordChangeDate>
            <recordContentSource>ContentDM</recordContentSource>
        </recordInfo>
    </xsl:template>
</xsl:stylesheet>
